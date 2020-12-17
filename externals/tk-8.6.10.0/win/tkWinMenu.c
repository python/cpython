/*
 * tkWinMenu.c --
 *
 *	This module implements the Windows platform-specific features of
 *	menus.
 *
 * Copyright (c) 1996-1998 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#define OEMRESOURCE
#include "tkWinInt.h"
#include "tkMenu.h"

/*
 * The class of the window for popup menus.
 */

#define MENU_CLASS_NAME			L"MenuWindowClass"
#define EMBEDDED_MENU_CLASS_NAME	L"EmbeddedMenuWindowClass"

/*
 * Used to align a windows bitmap inside a rectangle
 */

#define ALIGN_BITMAP_LEFT	0x00000001
#define ALIGN_BITMAP_RIGHT	0x00000002
#define ALIGN_BITMAP_TOP	0x00000004
#define ALIGN_BITMAP_BOTTOM	0x00000008


/*
 * Platform-specific menu flags:
 *
 * MENU_SYSTEM_MENU	Non-zero means that the Windows menu handle was
 *			retrieved with GetSystemMenu and needs to be disposed
 *			of specially.
 * MENU_RECONFIGURE_PENDING
 *			Non-zero means that an idle handler has been set up to
 *			reconfigure the Windows menu handle for this menu.
 */

#define MENU_SYSTEM_MENU		MENU_PLATFORM_FLAG1
#define MENU_RECONFIGURE_PENDING	MENU_PLATFORM_FLAG2

/*
 * ODS_NOACCEL flag forbids drawing accelerator cues (i.e. underlining labels)
 * on Windows 2000 and above.  The ODS_NOACCEL define is missing from mingw32
 * headers and undefined for _WIN32_WINNT < 0x0500 in Microsoft SDK.  We might
 * check for _WIN32_WINNT here, but I think it's not needed, as checking for
 * this flag does no harm on even on NT: reserved bits should be zero, and in
 * fact they are.
 */

#ifndef ODS_NOACCEL
#define ODS_NOACCEL 0x100
#endif
#ifndef SPI_GETKEYBOARDCUES
#define SPI_GETKEYBOARDCUES             0x100A
#endif
#ifndef WM_UPDATEUISTATE
#define WM_UPDATEUISTATE                0x0128
#endif
#ifndef UIS_SET
#define UIS_SET                         1
#endif
#ifndef UIS_CLEAR
#define UIS_CLEAR                       2
#endif
#ifndef UISF_HIDEACCEL
#define UISF_HIDEACCEL                  2
#endif

#ifndef WM_UNINITMENUPOPUP
#define WM_UNINITMENUPOPUP              0x0125
#endif

static int indicatorDimensions[2];
				/* The dimensions of the indicator space in a
				 * menu entry. Calculated at init time to save
				 * time. */

static BOOL showMenuAccelerators;

typedef struct {
    int inPostMenu;		/* We cannot be re-entrant like X Windows. */
    WORD lastCommandID;		/* The last command ID we allocated. */
    HWND menuHWND;		/* A window to service popup-menu messages
				 * in. */
    HWND embeddedMenuHWND;	/* A window to service embedded menu
				 * messages */
    int oldServiceMode;		/* Used while processing a menu; we need to
				 * set the event mode specially when we enter
				 * the menu processing modal loop and reset it
				 * when menus go away. */
    TkMenu *modalMenuPtr;	/* The menu we are processing inside the modal
				 * loop. We need this to reset all of the
				 * active items when menus go away since
				 * Windows does not see fit to give this to us
				 * when it sends its WM_MENUSELECT. */
    Tcl_HashTable commandTable;	/* A map of command ids to menu entries */
    Tcl_HashTable winMenuTable;	/* Need this to map HMENUs back to menuPtrs */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The following are default menu value strings.
 */

static int defaultBorderWidth;	/* The windows default border width. */
static Tcl_DString menuFontDString;
				/* A buffer to store the default menu font
				 * string. */
/*
 * Forward declarations for functions defined later in this file:
 */

static void		DrawMenuEntryAccelerator(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, const Tk_FontMetrics *fmPtr,
			    Tk_3DBorder activeBorder, int x, int y,
			    int width, int height);
static void		DrawMenuEntryArrow(TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, GC gc, Tk_3DBorder activeBorder,
			    int x,int y, int width, int height, int drawArrow);
static void		DrawMenuEntryBackground(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d,
			    Tk_3DBorder activeBorder, Tk_3DBorder bgBorder,
			    int x, int y, int width, int heigth);
static void		DrawMenuEntryIndicator(TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    GC indicatorGC, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height);
static void		DrawMenuEntryLabel(TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, GC gc, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height, int underline);
static void		DrawMenuSeparator(TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, GC gc, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr,
			    int x, int y, int width, int height);
static void		DrawTearoffEntry(TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, GC gc, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height);
static void		DrawMenuUnderline(TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, GC gc, Tk_Font tkfont,
			    const Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height);
static void		DrawWindowsSystemBitmap(Display *display,
			    Drawable drawable, GC gc, const RECT *rectPtr,
			    int bitmapID, int alignFlags);
static void		FreeID(WORD commandID);
static char *		GetEntryText(TkMenu *menuPtr, TkMenuEntry *mePtr);
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
static int		GetNewID(TkMenuEntry *mePtr, WORD *menuIDPtr);
static int		TkWinMenuKeyObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		MenuSelectEvent(TkMenu *menuPtr);
static void		ReconfigureWindowsMenu(ClientData clientData);
static void		RecursivelyClearActiveMenu(TkMenu *menuPtr);
static void		SetDefaults(int firstTime);
static LRESULT CALLBACK	TkWinMenuProc(HWND hwnd, UINT message, WPARAM wParam,
			    LPARAM lParam);
static LRESULT CALLBACK	TkWinEmbeddedMenuProc(HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam);

static inline void
ScheduleMenuReconfigure(
    TkMenu *menuPtr)
{
    if (!(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
	menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
	Tcl_DoWhenIdle(ReconfigureWindowsMenu, menuPtr);
    }
}

static inline void
CallPendingReconfigureImmediately(
    TkMenu *menuPtr)
{
    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
	Tcl_CancelIdleCall(ReconfigureWindowsMenu, menuPtr);
	ReconfigureWindowsMenu(menuPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetNewID --
 *
 *	Allocates a new menu id and marks it in use.
 *
 * Results:
 *	Returns TCL_OK if succesful; TCL_ERROR if there are no more ids of the
 *	appropriate type to allocate. menuIDPtr contains the new id if
 *	succesful.
 *
 * Side effects:
 *	An entry is created for the menu in the command hash table, and the
 *	hash entry is stored in the appropriate field in the menu data
 *	structure.
 *
 *----------------------------------------------------------------------
 */

static int
GetNewID(
    TkMenuEntry *mePtr,		/* The menu we are working with. */
    WORD *menuIDPtr)		/* The resulting id. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    WORD curID = tsdPtr->lastCommandID;

    while (1) {
	Tcl_HashEntry *commandEntryPtr;
	int new;

	/*
	 * Try the next ID number, taking care to wrap rather than stray
	 * into the system menu IDs.  [Bug 3235256]
	 */
	if (++curID >= 0xF000) {
	    curID = 1;
	}

	/* Return error when we've checked all IDs without success. */
	if (curID == tsdPtr->lastCommandID) {
	    return TCL_ERROR;
	}

	commandEntryPtr = Tcl_CreateHashEntry(&tsdPtr->commandTable,
		INT2PTR(curID), &new);
	if (new) {
	    Tcl_SetHashValue(commandEntryPtr, mePtr);
	    *menuIDPtr = curID;
	    tsdPtr->lastCommandID = curID;
	    return TCL_OK;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeID --
 *
 *	Marks the itemID as free.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table entry for the ID is cleared.
 *
 *----------------------------------------------------------------------
 */

static void
FreeID(
    WORD commandID)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    /*
     * If the menuHWND is NULL, this table has been finalized already.
     */

    if (tsdPtr->menuHWND != NULL) {
	Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&tsdPtr->commandTable,
		INT2PTR(commandID));

	if (entryPtr != NULL) {
	    Tcl_DeleteHashEntry(entryPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpNewMenu --
 *
 *	Gets a new blank menu. Only the platform specific options are filled
 *	in.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	Allocates a Windows menu handle and places it in the platformData
 *	field of the menuPtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpNewMenu(
    TkMenu *menuPtr)		/* The common structure we are making the
				 * platform structure for. */
{
    HMENU winMenuHdl;
    Tcl_HashEntry *hashEntryPtr;
    int newEntry;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    winMenuHdl = CreatePopupMenu();
    if (winMenuHdl == NULL) {
    	Tcl_SetObjResult(menuPtr->interp, Tcl_NewStringObj(
		"No more menus can be allocated.", -1));
	Tcl_SetErrorCode(menuPtr->interp, "TK", "MENU", "SYSTEM_RESOURCES", NULL);
    	return TCL_ERROR;
    }

    /*
     * We hash all of the HMENU's so that we can get their menu ptrs back when
     * dispatch messages.
     */

    hashEntryPtr = Tcl_CreateHashEntry(&tsdPtr->winMenuTable,
	    (char *) winMenuHdl, &newEntry);
    Tcl_SetHashValue(hashEntryPtr, menuPtr);

    menuPtr->platformData = (TkMenuPlatformData) winMenuHdl;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenu --
 *
 *	Destroys platform-specific menu structures.
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
    TkMenu *menuPtr)		/* The common menu structure */
{
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;
    const char *searchName;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
	Tcl_CancelIdleCall(ReconfigureWindowsMenu, menuPtr);
    }

    if (winMenuHdl == NULL) {
	return;
    }

    if (menuPtr->menuFlags & MENU_SYSTEM_MENU) {
	TkMenuEntry *searchEntryPtr;
	Tcl_HashTable *tablePtr = TkGetMenuHashTable(menuPtr->interp);
	char *menuName = Tcl_GetHashKey(tablePtr,
		menuPtr->menuRefPtr->hashEntryPtr);

	/*
	 * Search for the menu in the menubar, if it is present, get the
	 * wrapper window associated with the toplevel and reset its
	 * system menu to the default menu.
	 */

	for (searchEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
		searchEntryPtr != NULL;
		searchEntryPtr = searchEntryPtr->nextCascadePtr) {
	    searchName = Tcl_GetString(searchEntryPtr->namePtr);
	    if (strcmp(searchName, menuName) == 0) {
		Tk_Window parentTopLevelPtr = searchEntryPtr
			->menuPtr->parentTopLevelPtr;

		if (parentTopLevelPtr != NULL) {
		    GetSystemMenu(
			    TkWinGetWrapperWindow(parentTopLevelPtr), TRUE);
		}
		break;
	    }
	}
    } else {
	/*
	 * Remove the menu from the menu hash table, then destroy the handle.
	 * If the menuHWND is NULL, this table has been finalized already.
	 */

	if (tsdPtr->menuHWND != NULL) {
	    Tcl_HashEntry *hashEntryPtr =
		Tcl_FindHashEntry(&tsdPtr->winMenuTable, (char *) winMenuHdl);

	    if (hashEntryPtr != NULL) {
		Tcl_DeleteHashEntry(hashEntryPtr);
	    }
	}
 	DestroyMenu(winMenuHdl);
    }
    menuPtr->platformData = NULL;

    if (menuPtr == tsdPtr->modalMenuPtr) {
	tsdPtr->modalMenuPtr = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuEntry --
 *
 *	Cleans up platform-specific menu entry items.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	All platform-specific allocations are freed up.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenuEntry(
    TkMenuEntry *mePtr)		/* The entry to destroy */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;

    if (NULL != winMenuHdl) {
	ScheduleMenuReconfigure(menuPtr);
    }
    FreeID((WORD) PTR2INT(mePtr->platformEntryData));
    mePtr->platformEntryData = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * GetEntryText --
 *
 *	Given a menu entry, gives back the text that should go in it.
 *	Separators should be done by the caller, as they have to be handled
 *	specially. Allocates the memory with alloc. The caller should free the
 *	memory.
 *
 * Results:
 *	itemText points to the new text for the item.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
GetEntryText(
    TkMenu *menuPtr,		/* The menu considered. */
    TkMenuEntry *mePtr)		/* A pointer to the menu entry. */
{
    char *itemText;

    if (mePtr->type == TEAROFF_ENTRY) {
	itemText = ckalloc(sizeof("(Tear-off)"));
	strcpy(itemText, "(Tear-off)");
    } else if (mePtr->imagePtr != NULL) {
	itemText = ckalloc(sizeof("(Image)"));
	strcpy(itemText, "(Image)");
    } else if (mePtr->bitmapPtr != NULL) {
	itemText = ckalloc(sizeof("(Pixmap)"));
	strcpy(itemText, "(Pixmap)");
    } else if (mePtr->labelPtr == NULL || mePtr->labelLength == 0) {
	itemText = ckalloc(sizeof("( )"));
	strcpy(itemText, "( )");
    } else {
	int i;
	const char *label = (mePtr->labelPtr == NULL) ? ""
		: Tcl_GetString(mePtr->labelPtr);
	const char *accel = ((menuPtr->menuType == MENUBAR) || (mePtr->accelPtr == NULL)) ? ""
		: Tcl_GetString(mePtr->accelPtr);
	const char *p, *next;
	Tcl_DString itemString;

	/*
	 * We have to construct the string with an ampersand preceeding the
	 * underline character, and a tab seperating the text and the accel
	 * text. We have to be careful with ampersands in the string.
	 */

	Tcl_DStringInit(&itemString);

	for (p = label, i = 0; *p != '\0'; i++, p = next) {
	    if (i == mePtr->underline) {
		Tcl_DStringAppend(&itemString, "&", 1);
	    }
	    if (*p == '&') {
		Tcl_DStringAppend(&itemString, "&", 1);
	    }
	    next = Tcl_UtfNext(p);
	    Tcl_DStringAppend(&itemString, p, (int) (next - p));
	}
	if (mePtr->accelLength > 0) {
	    Tcl_DStringAppend(&itemString, "\t", 1);
	    for (p = accel, i = 0; *p != '\0'; i++, p = next) {
		if (*p == '&') {
		    Tcl_DStringAppend(&itemString, "&", 1);
		}
		next = Tcl_UtfNext(p);
		Tcl_DStringAppend(&itemString, p, (int) (next - p));
	    }
	}

	itemText = ckalloc(Tcl_DStringLength(&itemString) + 1);
	strcpy(itemText, Tcl_DStringValue(&itemString));
	Tcl_DStringFree(&itemString);
    }
    return itemText;
}

/*
 *----------------------------------------------------------------------
 *
 * ReconfigureWindowsMenu --
 *
 *	Tears down and rebuilds the platform-specific part of this menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configuration information get set for mePtr; old resources get freed,
 *	if any need it.
 *
 *----------------------------------------------------------------------
 */

static void
ReconfigureWindowsMenu(
    ClientData clientData)	/* The menu we are rebuilding */
{
    TkMenu *menuPtr = clientData;
    TkMenuEntry *mePtr;
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;
    char *itemText = NULL;
    LPCWSTR lpNewItem;
    UINT flags;
    UINT itemID;
    int i, count, systemMenu = 0, base;
    Tcl_DString translatedText;

    if (NULL == winMenuHdl) {
    	return;
    }

    /*
     * Reconstruct the entire menu. Takes care of nasty system menu and index
     * problem.
     */

    base = (menuPtr->menuFlags & MENU_SYSTEM_MENU) ? 7 : 0;
    count = GetMenuItemCount(winMenuHdl);
    for (i = base; i < count; i++) {
	RemoveMenu(winMenuHdl, base, MF_BYPOSITION);
    }

    count = menuPtr->numEntries;
    for (i = 0; i < count; i++) {
	mePtr = menuPtr->entries[i];
	lpNewItem = NULL;
	flags = MF_BYPOSITION;
	itemID = 0;
	Tcl_DStringInit(&translatedText);

	if ((menuPtr->menuType == MENUBAR) && (mePtr->type == TEAROFF_ENTRY)) {
	    continue;
	}

	itemText = GetEntryText(menuPtr, mePtr);
	if ((menuPtr->menuType == MENUBAR)
		|| (menuPtr->menuFlags & MENU_SYSTEM_MENU)) {
	    Tcl_WinUtfToTChar(itemText, -1, &translatedText);
	    lpNewItem = (LPCWSTR) Tcl_DStringValue(&translatedText);
	    flags |= MF_STRING;
	} else {
	    lpNewItem = (LPCWSTR) mePtr;
	    flags |= MF_OWNERDRAW;
	}

	/*
	 * Set enabling and disabling correctly.
	 */

	if (mePtr->state == ENTRY_DISABLED) {
	    flags |= MF_DISABLED | MF_GRAYED;
	}

	/*
	 * Set the check mark for check entries and radio entries.
	 */

	if (((mePtr->type == CHECK_BUTTON_ENTRY)
		|| (mePtr->type == RADIO_BUTTON_ENTRY))
		&& (mePtr->entryFlags & ENTRY_SELECTED)) {
	    flags |= MF_CHECKED;
	}

	/*
	 * Set the SEPARATOR bit for separator entries. This bit is not used
	 * by our internal drawing functions, but it is used by the system
	 * when drawing the system menu (we do not draw the system menu
	 * ourselves). If this bit is not set, separator entries on the system
	 * menu will not be drawn correctly.
	 */

	if (mePtr->type == SEPARATOR_ENTRY) {
	    flags |= MF_SEPARATOR;
	}

	if (mePtr->columnBreak) {
	    flags |= MF_MENUBREAK;
	}

	itemID = PTR2INT(mePtr->platformEntryData);
	if ((mePtr->type == CASCADE_ENTRY)
		&& (mePtr->childMenuRefPtr != NULL)
		&& (mePtr->childMenuRefPtr->menuPtr != NULL)) {
	    HMENU childMenuHdl = (HMENU) mePtr->childMenuRefPtr->menuPtr
		->platformData;
	    if (childMenuHdl != NULL) {
		/*
		 * Win32 draws the popup arrow in the wrong color for a
		 * disabled cascade menu, so do it by hand. Given it is
		 * disabled, there's no need for it to be connected to its
		 * child.
		 */

		if (mePtr->state != ENTRY_DISABLED) {
		    flags |= MF_POPUP;
		    /*
		     * If the MF_POPUP flag is set, then the id is interpreted
		     * as the handle of a submenu.
		     */
		    itemID = PTR2INT(childMenuHdl);
		}
	    }
	    if ((menuPtr->menuType == MENUBAR)
		    && !(mePtr->childMenuRefPtr->menuPtr->menuFlags
			    & MENU_SYSTEM_MENU)) {
		Tcl_DString ds;
		TkMenuReferences *menuRefPtr;
		TkMenu *systemMenuPtr = mePtr->childMenuRefPtr->menuPtr;

		Tcl_DStringInit(&ds);
		Tcl_DStringAppend(&ds,
			Tk_PathName(menuPtr->masterMenuPtr->tkwin), -1);
		Tcl_DStringAppend(&ds, ".system", 7);

		menuRefPtr = TkFindMenuReferences(menuPtr->interp,
			Tcl_DStringValue(&ds));

		Tcl_DStringFree(&ds);

		if ((menuRefPtr != NULL)
			&& (menuRefPtr->menuPtr != NULL)
			&& (menuPtr->parentTopLevelPtr != NULL)
			&& (systemMenuPtr->masterMenuPtr
				== menuRefPtr->menuPtr)) {
		    HMENU systemMenuHdl = (HMENU) systemMenuPtr->platformData;
		    HWND wrapper = TkWinGetWrapperWindow(menuPtr
			    ->parentTopLevelPtr);

		    if (wrapper != NULL) {
			DestroyMenu(systemMenuHdl);
			systemMenuHdl = GetSystemMenu(wrapper, FALSE);
			systemMenuPtr->menuFlags |= MENU_SYSTEM_MENU;
			systemMenuPtr->platformData =
				(TkMenuPlatformData) systemMenuHdl;
			ScheduleMenuReconfigure(systemMenuPtr);
		    }
		}
	    }
	    if (mePtr->childMenuRefPtr->menuPtr->menuFlags
		    & MENU_SYSTEM_MENU) {
		systemMenu++;
	    }
	}
	if (!systemMenu) {
	    InsertMenuW(winMenuHdl, 0xFFFFFFFF, flags, itemID, lpNewItem);
	}
	Tcl_DStringFree(&translatedText);
	if (itemText != NULL) {
	    ckfree(itemText);
	    itemText = NULL;
	}
    }


    if ((menuPtr->menuType == MENUBAR)
	    && (menuPtr->parentTopLevelPtr != NULL)) {
	HANDLE bar = TkWinGetWrapperWindow(menuPtr->parentTopLevelPtr);

	if (bar) {
	    DrawMenuBar(bar);
	}
    }

    menuPtr->menuFlags &= ~(MENU_RECONFIGURE_PENDING);
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
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;
    int result, flags;
    RECT noGoawayRect;
    POINT point;
    Tk_Window parentWindow = Tk_Parent(menuPtr->tkwin);
    int oldServiceMode = Tcl_GetServiceMode();
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->inPostMenu++;
    CallPendingReconfigureImmediately(menuPtr);

    result = TkPreprocessMenu(menuPtr);
    if (result != TCL_OK) {
	tsdPtr->inPostMenu--;
	return result;
    }

    if (index >= menuPtr->numEntries) {
	index = menuPtr->numEntries - 1;
    }
    if (index >= 0) {
	y -= menuPtr->entries[index]->y;
    }

    /*
     * The post commands could have deleted the menu, which means
     * we are dead and should go away.
     */

    if (menuPtr->tkwin == NULL) {
	tsdPtr->inPostMenu--;
    	return TCL_OK;
    }

    if (NULL == parentWindow) {
	noGoawayRect.top = y - 50;
	noGoawayRect.bottom = y + 50;
	noGoawayRect.left = x - 50;
	noGoawayRect.right = x + 50;
    } else {
	int left, top;
	Tk_GetRootCoords(parentWindow, &left, &top);
	noGoawayRect.left = left;
	noGoawayRect.top = top;
	noGoawayRect.bottom = noGoawayRect.top + Tk_Height(parentWindow);
	noGoawayRect.right = noGoawayRect.left + Tk_Width(parentWindow);
    }

    Tcl_SetServiceMode(TCL_SERVICE_NONE);

    /*
     * Make an assumption here. If the right button is down,
     * then we want to track it. Otherwise, track the left mouse button.
     */

    flags = TPM_LEFTALIGN;
    if (GetSystemMetrics(SM_SWAPBUTTON)) {
	if (GetAsyncKeyState(VK_LBUTTON) < 0) {
	    flags |= TPM_RIGHTBUTTON;
	} else {
	    flags |= TPM_LEFTBUTTON;
	}
    } else {
	if (GetAsyncKeyState(VK_RBUTTON) < 0) {
	    flags |= TPM_RIGHTBUTTON;
	} else {
	    flags |= TPM_LEFTBUTTON;
	}
    }

    TrackPopupMenu(winMenuHdl, flags, x, y, 0,
	    tsdPtr->menuHWND, &noGoawayRect);
    Tcl_SetServiceMode(oldServiceMode);

    GetCursorPos(&point);
    Tk_PointerEvent(NULL, point.x, point.y);

    if (tsdPtr->inPostMenu) {
	tsdPtr->inPostMenu = 0;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPostTearoffMenu --
 *
 *	Posts a tearoff menu on the screen so that the top left corner of the
 *      specified entry is located at the point (x, y) in screen coordinates.
 *      If the index parameter is negative, the upper left corner of the menu
 *      itself is placed at the point.  Adjusts the menu's position so that it
 *      fits on the screen, and maps and raises the menu.
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
    int x, int y, int index)	/* The root X,Y coordinates where we are
				 * posting */
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
 * TkpMenuNewEntry --
 *
 *	Adds a pointer to a new menu entry structure with the platform-
 *	specific fields filled in.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	A new command ID is allocated and stored in the platformEntryData
 *	field of mePtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpMenuNewEntry(
    TkMenuEntry *mePtr)
{
    WORD commandID;
    TkMenu *menuPtr = mePtr->menuPtr;

    if (GetNewID(mePtr, &commandID) != TCL_OK) {
    	return TCL_ERROR;
    }
    ScheduleMenuReconfigure(menuPtr);
    mePtr->platformEntryData = (TkMenuPlatformEntryData) INT2PTR(commandID);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinMenuProc --
 *
 *	The window proc for the dummy window we put popups in. This allows
 *	is to post a popup whether or not we know what the parent window
 *	is.
 *
 * Results:
 *	Returns whatever is appropriate for the message in question.
 *
 * Side effects:
 *	Normal side-effect for windows messages.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
TkWinMenuProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT lResult;

    if (!TkWinHandleMenuEvent(&hwnd, &message, &wParam, &lParam, &lResult)) {
	lResult = DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return lResult;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateEmbeddedMenu --
 *
 *	This function is used as work-around for updating the pull-down window
 *	of an embedded menu which may show as a blank popup window.
 *
 * Results:
 *	Invalidate the client area of the embedded pull-down menu and
 *	redraw it.
 *
 * Side effects:
 *	Redraw the embedded menu window.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateEmbeddedMenu(
    ClientData clientData)
{
    RECT rc;
    HWND hMenuWnd = (HWND)clientData;

    GetClientRect(hMenuWnd, &rc);
    InvalidateRect(hMenuWnd, &rc, FALSE);
    UpdateWindow(hMenuWnd);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinEmbeddedMenuProc --
 *
 *	This window proc is for the embedded menu windows. It provides
 *	message services to an embedded menu in a different process.
 *
 * Results:
 *	Returns 1 if the message has been handled or 0 otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
TkWinEmbeddedMenuProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    static int nIdles = 0;
    LRESULT lResult = 1;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    switch(message) {
    case WM_ENTERIDLE:
	if ((wParam == MSGF_MENU) && (nIdles < 1)
		&& (hwnd == tsdPtr->embeddedMenuHWND)) {
	    Tcl_CreateTimerHandler(200, UpdateEmbeddedMenu,
		    (ClientData) lParam);
	    nIdles++;
	}
	break;

    case WM_INITMENUPOPUP:
	nIdles = 0;
	break;

    case WM_SETTINGCHANGE:
	if (wParam == SPI_SETNONCLIENTMETRICS
		|| wParam == SPI_SETKEYBOARDCUES) {
	    SetDefaults(0);
	}
	break;

    case WM_INITMENU:
    case WM_SYSCOMMAND:
    case WM_COMMAND:
    case WM_MENUCHAR:
    case WM_MEASUREITEM:
    case WM_DRAWITEM:
    case WM_MENUSELECT:
	lResult = TkWinHandleMenuEvent(&hwnd, &message, &wParam, &lParam,
		&lResult);
	if (lResult || (GetCapture() != hwnd)) {
	    break;
	}
	/* FALLTHRU */
    default:
	lResult = DefWindowProcW(hwnd, message, wParam, lParam);
	break;
    }
    return lResult;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinHandleMenuEvent --
 *
 *	Filters out menu messages from messages passed to a top-level. Will
 *	respond appropriately to WM_COMMAND, WM_MENUSELECT, WM_MEASUREITEM,
 *	WM_DRAWITEM
 *
 * Result:
 *	Returns 1 if this handled the message; 0 if it did not.
 *
 * Side effects:
 *	All of the parameters may be modified so that the caller can think it
 *	is getting a different message. plResult points to the result that
 *	should be returned to windows from this message.
 *
 *----------------------------------------------------------------------
 */

int
TkWinHandleMenuEvent(
    HWND *phwnd,
    UINT *pMessage,
    WPARAM *pwParam,
    LPARAM *plParam,
    LRESULT *plResult)
{
    Tcl_HashEntry *hashEntryPtr;
    int returnResult = 0;
    TkMenu *menuPtr;
    TkMenuEntry *mePtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    switch (*pMessage) {
    case WM_UNINITMENUPOPUP:
	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable,
		(char *) *pwParam);
	if (hashEntryPtr != NULL) {
	    menuPtr = Tcl_GetHashValue(hashEntryPtr);
	    if ((menuPtr->menuRefPtr != NULL)
		    && (menuPtr->menuRefPtr->parentEntryPtr != NULL)) {
		TkPostSubmenu(menuPtr->interp,
			menuPtr->menuRefPtr->parentEntryPtr->menuPtr, NULL);
	    }
	}
	break;

    case WM_INITMENU:
	TkMenuInit();
	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable,
		(char *) *pwParam);
	if (hashEntryPtr != NULL) {
	    tsdPtr->oldServiceMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
	    menuPtr = Tcl_GetHashValue(hashEntryPtr);
	    tsdPtr->modalMenuPtr = menuPtr;
	    CallPendingReconfigureImmediately(menuPtr);
	    RecursivelyClearActiveMenu(menuPtr);
	    if (!tsdPtr->inPostMenu) {
		Tcl_Interp *interp = menuPtr->interp;
		int code;

		Tcl_Preserve(interp);
		code = TkPreprocessMenu(menuPtr);
		if ((code != TCL_OK) && (code != TCL_CONTINUE)
			&& (code != TCL_BREAK)) {
		    Tcl_AddErrorInfo(interp, "\n    (menu preprocess)");
		    Tcl_BackgroundException(interp, code);
		}
		Tcl_Release(interp);
	    }
	    TkActivateMenuEntry(menuPtr, -1);
	    *plResult = 0;
	    returnResult = 1;
	} else {
	    tsdPtr->modalMenuPtr = NULL;
	}
	break;

    case WM_SYSCOMMAND:
    case WM_COMMAND:
	TkMenuInit();
	if (HIWORD(*pwParam) != 0) {
	    break;
	}
	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->commandTable,
		INT2PTR(LOWORD(*pwParam)));
	if (hashEntryPtr == NULL) {
	    break;
	}
	mePtr = Tcl_GetHashValue(hashEntryPtr);
	if (mePtr != NULL) {
	    TkMenuReferences *menuRefPtr;
	    TkMenuEntry *parentEntryPtr;
	    Tcl_Interp *interp;
	    int code;

	    /*
	     * We have to set the parent of this menu to be active if this is
	     * a submenu so that tearoffs will get the correct title.
	     */

	    menuPtr = mePtr->menuPtr;
	    menuRefPtr = TkFindMenuReferences(menuPtr->interp,
		    Tk_PathName(menuPtr->tkwin));
	    if ((menuRefPtr != NULL) && (menuRefPtr->parentEntryPtr != NULL)) {
		for (parentEntryPtr = menuRefPtr->parentEntryPtr ; ;
			parentEntryPtr = parentEntryPtr->nextCascadePtr) {
		    const char *name = Tcl_GetString(parentEntryPtr->namePtr);

		    if (strcmp(name, Tk_PathName(menuPtr->tkwin)) == 0) {
			break;
		    }
		}
		if (parentEntryPtr->menuPtr->entries[parentEntryPtr->index]
			->state != ENTRY_DISABLED) {
		    TkActivateMenuEntry(parentEntryPtr->menuPtr,
			    parentEntryPtr->index);
		}
	    }

	    interp = menuPtr->interp;
	    Tcl_Preserve(interp);
	    code = TkInvokeMenu(interp, menuPtr, mePtr->index);
	    if (code != TCL_OK && code != TCL_CONTINUE && code != TCL_BREAK) {
		Tcl_AddErrorInfo(interp, "\n    (menu invoke)");
		Tcl_BackgroundException(interp, code);
	    }
	    Tcl_Release(interp);
	    *plResult = 0;
	    returnResult = 1;
	}
	break;

    case WM_MENUCHAR: {
	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable,
		(char *) *plParam);
	if (hashEntryPtr != NULL) {
	    int i, len, underline;
	    Tcl_Obj *labelPtr;
	    LPCWSTR wlabel;
	    int menuChar;
	    Tcl_DString ds;

	    *plResult = 0;
	    menuPtr = Tcl_GetHashValue(hashEntryPtr);
	    /*
	     * Assume we have something directly convertable to Tcl_UniChar.
	     * True at least for wide systems.
	     */
	    menuChar = Tcl_UniCharToUpper(LOWORD(*pwParam));

	    Tcl_DStringInit(&ds);
	    for (i = 0; i < menuPtr->numEntries; i++) {
		underline = menuPtr->entries[i]->underline;
		labelPtr = menuPtr->entries[i]->labelPtr;
		if ((underline >= 0) && (labelPtr != NULL)) {
		    /*
		     * Ensure we don't exceed the label length, then check
		     */
		    const char *src = Tcl_GetStringFromObj(labelPtr, &len);

		    Tcl_DStringFree(&ds);
		    wlabel = (LPCWSTR) Tcl_WinUtfToTChar(src, len, &ds);
		    if ((underline < len) && (menuChar ==
				Tcl_UniCharToUpper(wlabel[underline]))) {
			*plResult = (2 << 16) | i;
			returnResult = 1;
			break;
		    }
		}
	    }
	    Tcl_DStringFree(&ds);
	}
	break;
    }

    case WM_MEASUREITEM: {
	LPMEASUREITEMSTRUCT itemPtr = (LPMEASUREITEMSTRUCT) *plParam;

	if (itemPtr != NULL && tsdPtr->modalMenuPtr != NULL) {
	    mePtr = (TkMenuEntry *) itemPtr->itemData;
	    menuPtr = mePtr->menuPtr;

	    TkRecomputeMenu(menuPtr);
	    itemPtr->itemHeight = mePtr->height;
	    itemPtr->itemWidth = mePtr->width;
	    if (mePtr->hideMargin) {
		itemPtr->itemWidth += 2 - indicatorDimensions[1];
	    } else {
		int activeBorderWidth;

		Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
			menuPtr->activeBorderWidthPtr, &activeBorderWidth);
		itemPtr->itemWidth += 2 * activeBorderWidth;
	    }
	    *plResult = 1;
	    returnResult = 1;
	}
	break;
    }

    case WM_DRAWITEM: {
	TkWinDrawable *twdPtr;
	LPDRAWITEMSTRUCT itemPtr = (LPDRAWITEMSTRUCT) *plParam;
	Tk_FontMetrics fontMetrics;
	int drawingParameters = 0;

	if (itemPtr != NULL && tsdPtr->modalMenuPtr != NULL) {
	    Tk_Font tkfont;

	    if (itemPtr->itemState & ODS_NOACCEL && !showMenuAccelerators) {
		drawingParameters |= DRAW_MENU_ENTRY_NOUNDERLINE;
	    }
	    mePtr = (TkMenuEntry *) itemPtr->itemData;
	    menuPtr = mePtr->menuPtr;
	    twdPtr = ckalloc(sizeof(TkWinDrawable));
	    twdPtr->type = TWD_WINDC;
	    twdPtr->winDC.hdc = itemPtr->hDC;

	    if (mePtr->state != ENTRY_DISABLED) {
		if (itemPtr->itemState & ODS_SELECTED) {
		    TkActivateMenuEntry(menuPtr, mePtr->index);
		} else {
		    TkActivateMenuEntry(menuPtr, -1);
		}
	    } else {
		/*
		 * On windows, menu entries should highlight even if they are
		 * disabled. (I know this seems dumb, but it is the way native
		 * windows menus works so we ought to mimic it.) The
		 * ENTRY_PLATFORM_FLAG1 flag will indicate that the entry
		 * should be highlighted even though it is disabled.
		 */

		if (itemPtr->itemState & ODS_SELECTED) {
		    mePtr->entryFlags |= ENTRY_PLATFORM_FLAG1;
		} else {
		    mePtr->entryFlags &= ~ENTRY_PLATFORM_FLAG1;
		}

		/*
		 * Also, set the DRAW_MENU_ENTRY_ARROW flag for a disabled
		 * cascade menu since we need to draw the arrow ourselves.
		 */

		if (mePtr->type == CASCADE_ENTRY) {
		    drawingParameters |= DRAW_MENU_ENTRY_ARROW;
		}
	    }

	    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
	    Tk_GetFontMetrics(tkfont, &fontMetrics);
	    TkpDrawMenuEntry(mePtr, (Drawable) twdPtr, tkfont, &fontMetrics,
		    itemPtr->rcItem.left, itemPtr->rcItem.top,
		    itemPtr->rcItem.right - itemPtr->rcItem.left,
		    itemPtr->rcItem.bottom - itemPtr->rcItem.top,
		    0, drawingParameters);

	    ckfree(twdPtr);
	}
	*plResult = 1;
	returnResult = 1;
	break;
    }

    case WM_MENUSELECT: {
	UINT flags = HIWORD(*pwParam);

	TkMenuInit();

	if ((flags == 0xFFFF) && (*plParam == 0)) {
	    if (tsdPtr->modalMenuPtr != NULL) {
		Tcl_SetServiceMode(tsdPtr->oldServiceMode);
		RecursivelyClearActiveMenu(tsdPtr->modalMenuPtr);
	    }
	} else {
	    menuPtr = NULL;
	    if (*plParam != 0) {
		hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable,
			(char *) *plParam);
		if (hashEntryPtr != NULL) {
		    menuPtr = Tcl_GetHashValue(hashEntryPtr);
		}
	    }

	    if (menuPtr != NULL) {
		long entryIndex = LOWORD(*pwParam);

                if ((menuPtr->menuType == MENUBAR) && menuPtr->tearoff) {
                    /*
                     * Windows passes the entry index starting at 0 for
                     * the first menu entry. However this entry #0 is the
                     * tearoff entry for Tk (the menu has -tearoff 1),
                     * which is ignored for MENUBAR menues on Windows.
                     */

                    entryIndex++;
                }
                mePtr = NULL;
		if (flags != 0xFFFF) {
		    if ((flags&MF_POPUP) && (entryIndex<menuPtr->numEntries)) {
			mePtr = menuPtr->entries[entryIndex];
		    } else {
			hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->commandTable,
				INT2PTR(entryIndex));
			if (hashEntryPtr != NULL) {
			    mePtr = Tcl_GetHashValue(hashEntryPtr);
			}
		    }
		}

		if ((mePtr == NULL) || (mePtr->state == ENTRY_DISABLED)) {
		    TkActivateMenuEntry(menuPtr, -1);
		} else {
		    if (mePtr->index >= menuPtr->numEntries) {
			Tcl_Panic("Trying to activate an entry which doesn't exist");
		    }
		    TkActivateMenuEntry(menuPtr, mePtr->index);
		}
		MenuSelectEvent(menuPtr);
		Tcl_ServiceAll();
		*plResult = 0;
		returnResult = 1;
	    }
	}
	break;
    }
    }
    return returnResult;
}

/*
 *----------------------------------------------------------------------
 *
 * RecursivelyClearActiveMenu --
 *
 *	Recursively clears the active entry in the menu's cascade hierarchy.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates <<MenuSelect>> virtual events.
 *
 *----------------------------------------------------------------------
 */

void
RecursivelyClearActiveMenu(
    TkMenu *menuPtr)		/* The menu to reset. */
{
    int i;
    TkMenuEntry *mePtr;

    TkActivateMenuEntry(menuPtr, -1);
    MenuSelectEvent(menuPtr);
    for (i = 0; i < menuPtr->numEntries; i++) {
    	mePtr = menuPtr->entries[i];
	if (mePtr->state == ENTRY_ACTIVE) {
	    mePtr->state = ENTRY_NORMAL;
	}
	mePtr->entryFlags &= ~ENTRY_PLATFORM_FLAG1;
    	if (mePtr->type == CASCADE_ENTRY) {
    	    if ((mePtr->childMenuRefPtr != NULL)
    	    	    && (mePtr->childMenuRefPtr->menuPtr != NULL)) {
    	    	RecursivelyClearActiveMenu(mePtr->childMenuRefPtr->menuPtr);
    	    }
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetWindowMenuBar --
 *
 *	Associates a given menu with a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	On Windows and UNIX, associates the platform menu with the
 *	platform window.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetWindowMenuBar(
    Tk_Window tkwin,		/* The window we are putting the menubar
				 * into.*/
    TkMenu *menuPtr)		/* The menu we are inserting */
{
    HMENU winMenuHdl;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (menuPtr != NULL) {
	Tcl_HashEntry *hashEntryPtr;
	int newEntry;

	winMenuHdl = (HMENU) menuPtr->platformData;
	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable,
		(char *) winMenuHdl);
	Tcl_DeleteHashEntry(hashEntryPtr);
	DestroyMenu(winMenuHdl);
	winMenuHdl = CreateMenu();
	hashEntryPtr = Tcl_CreateHashEntry(&tsdPtr->winMenuTable,
		(char *) winMenuHdl, &newEntry);
	Tcl_SetHashValue(hashEntryPtr, menuPtr);
	menuPtr->platformData = (TkMenuPlatformData) winMenuHdl;
	TkWinSetMenu(tkwin, winMenuHdl);
	ScheduleMenuReconfigure(menuPtr);
    } else {
	TkWinSetMenu(tkwin, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetMainMenubar --
 *
 *	Puts the menu associated with a window into the menubar. Should only
 *	be called when the window is in front.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menubar is changed.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetMainMenubar(
    Tcl_Interp *interp,		/* The interpreter of the application */
    Tk_Window tkwin,		/* The frame we are setting up */
    const char *menuName)	/* The name of the menu to put in front. If
    				 * NULL, use the default menu bar. */
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

void
GetMenuIndicatorGeometry(
    TkMenu *menuPtr,		/* The menu we are measuring */
    TkMenuEntry *mePtr,		/* The entry we are measuring */
    Tk_Font tkfont,		/* Precalculated font */
    const Tk_FontMetrics *fmPtr,/* Precalculated font metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    *heightPtr = indicatorDimensions[0];
    if (mePtr->hideMargin) {
	*widthPtr = 0;
    } else {
	int borderWidth;

	Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
		menuPtr->borderWidthPtr, &borderWidth);
	*widthPtr = indicatorDimensions[1] - borderWidth;

        /*
         * Quite dubious about the above (why would borderWidth play a role?)
         * and about how indicatorDimensions[1] is obtained in SetDefaults().
         * At least don't let the result be negative!
         */
        if (*widthPtr < 0) {
            *widthPtr = 0;
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuAccelGeometry --
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

void
GetMenuAccelGeometry(
    TkMenu *menuPtr,		/* The menu we are measuring */
    TkMenuEntry *mePtr,		/* The entry we are measuring */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    *heightPtr = fmPtr->linespace;
    if (mePtr->type == CASCADE_ENTRY) {
	*widthPtr = 0;
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

void
GetTearoffEntryGeometry(
    TkMenu *menuPtr,		/* The menu we are measuring */
    TkMenuEntry *mePtr,		/* The entry we are measuring */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    if (menuPtr->menuType != MASTER_MENU) {
	*heightPtr = 0;
    } else {
	*heightPtr = fmPtr->linespace;
    }
    *widthPtr = 0;
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

void
GetMenuSeparatorGeometry(
    TkMenu *menuPtr,		/* The menu we are measuring */
    TkMenuEntry *mePtr,		/* The entry we are measuring */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalcualted font metrics */
    int *widthPtr,		/* The resulting width */
    int *heightPtr)		/* The resulting height */
{
    *widthPtr = 0;
    *heightPtr = fmPtr->linespace - (2 * fmPtr->descent);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawWindowsSystemBitmap --
 *
 *	Draws the windows system bitmap given by bitmapID into the rect given
 *	by rectPtr in the drawable. The bitmap is centered in the rectangle.
 *	It is not clipped, so if the bitmap is bigger than the rect it will
 *	bleed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Drawing occurs. Some storage is allocated and released.
 *
 *----------------------------------------------------------------------
 */

static void
DrawWindowsSystemBitmap(
    Display *display,		/* The display we are drawing into */
    Drawable drawable,		/* The drawable we are working with */
    GC gc,			/* The GC to draw with */
    const RECT *rectPtr,	/* The rectangle to draw into */
    int bitmapID,		/* The windows id of the system bitmap to
				 * draw. */
    int alignFlags)		/* How to align the bitmap inside the
				 * rectangle. */
{
    TkWinDCState state;
    HDC hdc = TkWinGetDrawableDC(display, drawable, &state);
    HDC scratchDC;
    HBITMAP bitmap;
    BITMAP bm;
    POINT ptSize;
    POINT ptOrg;
    int topOffset, leftOffset;

    SetBkColor(hdc, gc->background);
    SetTextColor(hdc, gc->foreground);

    scratchDC = CreateCompatibleDC(hdc);
    bitmap = LoadBitmapW(NULL, (LPCWSTR)MAKEINTRESOURCE(bitmapID));

    SelectObject(scratchDC, bitmap);
    SetMapMode(scratchDC, GetMapMode(hdc));
    GetObjectA(bitmap, sizeof(BITMAP), &bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(scratchDC, &ptSize, 1);

    ptOrg.y = ptOrg.x = 0;
    DPtoLP(scratchDC, &ptOrg, 1);

    if (alignFlags & ALIGN_BITMAP_TOP) {
	topOffset = 0;
    } else if (alignFlags & ALIGN_BITMAP_BOTTOM) {
	topOffset = (rectPtr->bottom - rectPtr->top) - ptSize.y;
    } else {
	topOffset = (rectPtr->bottom - rectPtr->top) / 2 - (ptSize.y / 2);
    }

    if (alignFlags & ALIGN_BITMAP_LEFT) {
	leftOffset = 0;
    } else if (alignFlags & ALIGN_BITMAP_RIGHT) {
	leftOffset = (rectPtr->right - rectPtr->left) - ptSize.x;
    } else {
	leftOffset = (rectPtr->right - rectPtr->left) / 2 - (ptSize.x / 2);
    }

    BitBlt(hdc, rectPtr->left + leftOffset, rectPtr->top + topOffset, ptSize.x,
	    ptSize.y, scratchDC, ptOrg.x, ptOrg.y, SRCCOPY);
    DeleteDC(scratchDC);
    DeleteObject(bitmap);

    TkWinReleaseDrawableDC(drawable, hdc, &state);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryIndicator --
 *
 *	This function draws the indicator part of a menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its current mode.
 *
 *----------------------------------------------------------------------
 */

void
DrawMenuEntryIndicator(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* What we are drawing into */
    GC gc,			/* The gc we are drawing with */
    GC indicatorGC,		/* The gc for indicator objects */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int x,			/* Left edge */
    int y,			/* Top edge */
    int width,
    int height)
{
    if ((mePtr->type == CHECK_BUTTON_ENTRY)
	    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
    	if (mePtr->indicatorOn && (mePtr->entryFlags & ENTRY_SELECTED)) {
	    RECT rect;
	    GC whichGC;
	    int borderWidth, activeBorderWidth;

	    if (mePtr->state != ENTRY_NORMAL) {
		whichGC = gc;
	    } else {
		whichGC = indicatorGC;
	    }

	    rect.top = y;
	    rect.bottom = y + mePtr->height;
	    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
		    menuPtr->borderWidthPtr, &borderWidth);
	    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
		    menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	    rect.left = borderWidth + activeBorderWidth + x;
	    rect.right = mePtr->indicatorSpace + x;

	    if ((mePtr->state == ENTRY_DISABLED)
		    && (menuPtr->disabledFgPtr != NULL)) {
		RECT hilightRect;
		COLORREF oldFgColor = whichGC->foreground;

		whichGC->foreground = GetSysColor(COLOR_3DHILIGHT);
		hilightRect.top = rect.top + 1;
		hilightRect.bottom = rect.bottom + 1;
		hilightRect.left = rect.left + 1;
		hilightRect.right = rect.right + 1;
		DrawWindowsSystemBitmap(menuPtr->display, d, whichGC,
			&hilightRect, OBM_CHECK, 0);
		whichGC->foreground = oldFgColor;
	    }

	    DrawWindowsSystemBitmap(menuPtr->display, d, whichGC, &rect,
		    OBM_CHECK, 0);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryAccelerator --
 *
 *	This function draws the accelerator part of a menu. For example, the
 *	string "CTRL-Z" could be drawn to to the right of the label text for
 *	an Undo menu entry. Need to decide what to draw here. Should we
 *	replace strings like "Control", "Command", etc?
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

void
DrawMenuEntryAccelerator(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* What we are drawing into */
    GC gc,			/* The gc we are drawing with */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    Tk_3DBorder activeBorder,	/* The border when an item is active */
    int x,			/* left edge */
    int y,			/* top edge */
    int width,			/* Width of menu entry */
    int height)			/* Height of menu entry */
{
    int baseline;
    int leftEdge = x + mePtr->indicatorSpace + mePtr->labelWidth;
    const char *accel;

    if (menuPtr->menuType == MENUBAR) {
        return;
    }

    if (mePtr->accelPtr != NULL) {
	accel = Tcl_GetString(mePtr->accelPtr);
    } else {
	accel = NULL;
    }

    baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;

    /*
     * Draw disabled 3D text highlight only with the Win95/98 look.
     */

    if (TkWinGetPlatformTheme() == TK_THEME_WIN_CLASSIC) {
	if ((mePtr->state == ENTRY_DISABLED)
		&& (menuPtr->disabledFgPtr != NULL) && (accel != NULL)) {
	    COLORREF oldFgColor = gc->foreground;

	    gc->foreground = GetSysColor(COLOR_3DHILIGHT);
	    if (!(mePtr->entryFlags & ENTRY_PLATFORM_FLAG1)) {
		Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel,
			mePtr->accelLength, leftEdge + 1, baseline + 1);
	    }
	    gc->foreground = oldFgColor;
	}
    }

    if (accel != NULL) {
	Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel,
		mePtr->accelLength, leftEdge, baseline);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryArrow --
 *
 *	This function draws the arrow bitmap on the right side of a menu
 *	entry. This function is only used when drawing the arrow for a
 *	disabled cascade menu.
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
DrawMenuEntryArrow(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* What we are drawing into */
    GC gc,			/* The gc we are drawing with */
    Tk_3DBorder activeBorder,	/* The border when an item is active */
    int x,			/* left edge */
    int y,			/* top edge */
    int width,			/* Width of menu entry */
    int height,			/* Height of menu entry */
    int drawArrow)		/* For cascade menus, whether of not to draw
				 * the arrow. I cannot figure out Windows'
				 * algorithm for where to draw this. */
{
    COLORREF oldFgColor;
    COLORREF oldBgColor;
    RECT rect;

    if (!drawArrow || (mePtr->type != CASCADE_ENTRY)) {
	return;
    }

    oldFgColor = gc->foreground;
    oldBgColor = gc->background;

    /*
     * Set bitmap bg to highlight color if the menu is highlighted.
     */

    if (mePtr->entryFlags & ENTRY_PLATFORM_FLAG1) {
	XColor *activeBgColor = Tk_3DBorderColor(Tk_Get3DBorderFromObj(
		mePtr->menuPtr->tkwin, (mePtr->activeBorderPtr == NULL)
		? mePtr->menuPtr->activeBorderPtr
		: mePtr->activeBorderPtr));

	gc->background = activeBgColor->pixel;
    }

    gc->foreground = GetSysColor((mePtr->state == ENTRY_DISABLED)
	? COLOR_GRAYTEXT
		: ((mePtr->state == ENTRY_ACTIVE)
		? COLOR_HIGHLIGHTTEXT : COLOR_MENUTEXT));

    rect.top = y + GetSystemMetrics(SM_CYBORDER);
    rect.bottom = y + height - GetSystemMetrics(SM_CYBORDER);
    rect.left = x + mePtr->indicatorSpace + mePtr->labelWidth;
    rect.right = x + width;

    DrawWindowsSystemBitmap(menuPtr->display, d, gc, &rect, OBM_MNARROW,
	    ALIGN_BITMAP_RIGHT);

    gc->foreground = oldFgColor;
    gc->background = oldBgColor;
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuSeparator --
 *
 *	The menu separator is drawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its current mode.
 *
 *----------------------------------------------------------------------
 */

void
DrawMenuSeparator(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* What we are drawing into */
    GC gc,			/* The gc we are drawing with */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int x,			/* left edge */
    int y,			/* top edge */
    int width,			/* width of item */
    int height)			/* height of item */
{
    XPoint points[2];
    Tk_3DBorder border;

    points[0].x = x;
    points[0].y = y + height / 2;
    points[1].x = x + width - 1;
    points[1].y = points[0].y;
    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);
    Tk_Draw3DPolygon(menuPtr->tkwin, d, border, points, 2, 1,
	    TK_RELIEF_RAISED);
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
    int x,			/* Left Edge */
    int y,			/* Top Edge */
    int width,			/* Width of entry */
    int height)			/* Height of entry */
{
    if ((mePtr->underline >= 0) && (mePtr->labelPtr != NULL)) {
	int len;

	len = Tcl_GetCharLength(mePtr->labelPtr);
	if (mePtr->underline < len) {
	    const char *label, *start, *end;

	    label = Tcl_GetString(mePtr->labelPtr);
	    start = Tcl_UtfAtIndex(label, mePtr->underline);
	    end = Tcl_UtfNext(start);
	    Tk_UnderlineChars(menuPtr->display, d,
		    gc, tkfont, label, x + mePtr->indicatorSpace,
		    y + (height + fmPtr->ascent - fmPtr->descent) / 2,
		    (int) (start - label), (int) (end - label));
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkWinMenuKeyObjCmd --
 *
 *	This function is invoked when keys related to pulling down menus is
 *	pressed. The corresponding Windows events are generated and passed to
 *	DefWindowProcW if appropriate. This cmd is registered as tk::WinMenuKey
 *	in the interp.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	The menu system may take over and process user events for menu input.
 *
 *--------------------------------------------------------------
 */

static int
TkWinMenuKeyObjCmd(
    ClientData clientData,	/* Unused. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    UINT scanCode;
    UINT virtualKey;
    XEvent *eventPtr;
    Tk_Window tkwin;
    TkWindow *winPtr;
    KeySym keySym;
    int i;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "window keySym");
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[1]),
	    Tk_MainWindow(interp));

    if (tkwin == NULL) {
	/*
	 * If we don't find the key, just return, as the window may have
	 * been destroyed in the binding. [Bug 1236306]
	 */
	return TCL_OK;
    }

    eventPtr = TkpGetBindingXEvent(interp);

    winPtr = (TkWindow *)tkwin;

    if (Tcl_GetIntFromObj(interp, objv[2], &i) != TCL_OK) {
	return TCL_ERROR;
    }
    keySym = i;

    if (eventPtr->type == KeyPress) {
	switch (keySym) {
	case XK_Alt_L:
	    scanCode = MapVirtualKeyW(VK_LMENU, 0);
	    CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYDOWN, VK_MENU,
		    (int) (scanCode << 16) | (1 << 29));
	    break;
	case XK_Alt_R:
	    scanCode = MapVirtualKeyW(VK_RMENU, 0);
	    CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYDOWN, VK_MENU,
		    (int) (scanCode << 16) | (1 << 29) | (1 << 24));
	    break;
	case XK_F10:
	    scanCode = MapVirtualKeyW(VK_F10, 0);
	    CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYDOWN, VK_F10, (int) (scanCode << 16));
	    break;
	default:
	    virtualKey = XKeysymToKeycode(winPtr->display, keySym);
	    scanCode = MapVirtualKeyW(virtualKey, 0);
	    if (0 != scanCode) {
		XKeyEvent xkey = eventPtr->xkey;
		CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
			WM_SYSKEYDOWN, virtualKey,
			(int) ((scanCode << 16) | (1 << 29)));
		if (xkey.nbytes > 0) {
		    for (i = 0; i < xkey.nbytes; i++) {
			CallWindowProcW(DefWindowProcW,
				Tk_GetHWND(Tk_WindowId(tkwin)), WM_SYSCHAR,
				xkey.trans_chars[i],
				(int) ((scanCode << 16) | (1 << 29)));
		    }
		}
	    }
	}
    } else if (eventPtr->type == KeyRelease) {
	switch (keySym) {
	case XK_Alt_L:
	    scanCode = MapVirtualKeyW(VK_LMENU, 0);
	    CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYUP, VK_MENU, (int) (scanCode << 16)
		    | (1 << 29) | (1 << 30) | (1 << 31));
	    break;
	case XK_Alt_R:
	    scanCode = MapVirtualKeyW(VK_RMENU, 0);
	    CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYUP, VK_MENU, (int) (scanCode << 16) | (1 << 24)
		    | (1 << 29) | (1 << 30) | (1 << 31));
	    break;
	case XK_F10:
	    scanCode = MapVirtualKeyW(VK_F10, 0);
	    CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYUP, VK_F10,
		    (int) (scanCode << 16) | (1 << 30) | (1 << 31));
	    break;
	default:
	    virtualKey = XKeysymToKeycode(winPtr->display, keySym);
	    scanCode = MapVirtualKeyW(virtualKey, 0);
	    if (0 != scanCode) {
		CallWindowProcW(DefWindowProcW, Tk_GetHWND(Tk_WindowId(tkwin)),
			WM_SYSKEYUP, virtualKey, (int) ((scanCode << 16)
			| (1 << 29) | (1 << 30) | (1 << 31)));
	    }
	}
    }
    return TCL_OK;
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
 *	bindings are setup for the interp which will handle Alt-key sequences
 *	for menus without beeping or interfering with user-defined Alt-key
 *	bindings.
 *
 *--------------------------------------------------------------
 */

void
TkpInitializeMenuBindings(
    Tcl_Interp *interp,		/* The interpreter to set. */
    Tk_BindingTable bindingTable)
				/* The table to add to. */
{
    Tk_Uid uid = Tk_GetUid("all");

    /*
     * We need to set up the bindings for menubars. These have to recreate
     * windows events, so we need to invoke C code to generate the
     * WM_SYSKEYDOWNS and WM_SYSKEYUPs appropriately. Trick is, we can't
     * create a C level binding directly since we may want to modify the
     * binding in Tcl code.
     */

    (void) Tcl_CreateObjCommand(interp, "tk::WinMenuKey",
	    TkWinMenuKeyObjCmd,
	    (ClientData) Tk_MainWindow(interp), (Tcl_CmdDeleteProc *) NULL);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<Alt_L>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<KeyRelease-Alt_L>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<Alt_R>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<KeyRelease-Alt_R>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<Alt-KeyPress>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<Alt-KeyRelease>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<KeyPress-F10>", "tk::WinMenuKey %W %N", 0);

    (void) Tk_CreateBinding(interp, bindingTable, (ClientData) uid,
	    "<KeyRelease-F10>", "tk::WinMenuKey %W %N", 0);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryLabel --
 *
 *	This function draws the label part of a menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuEntryLabel(
    TkMenu *menuPtr,		/* The menu we are drawing */
    TkMenuEntry *mePtr,		/* The entry we are drawing */
    Drawable d,			/* What we are drawing into */
    GC gc,			/* The gc we are drawing into */
    Tk_Font tkfont,		/* The precalculated font */
    const Tk_FontMetrics *fmPtr,/* The precalculated font metrics */
    int x,			/* left edge */
    int y,			/* right edge */
    int width,			/* width of entry */
    int height,			/* height of entry */
    int underline)		/* accelerator cue should be drawn */
{
    int indicatorSpace = mePtr->indicatorSpace;
    int activeBorderWidth;
    int leftEdge;
    int imageHeight, imageWidth;
    int textHeight = 0, textWidth = 0;
    int haveImage = 0, haveText = 0;
    int imageXOffset = 0, imageYOffset = 0;
    int textXOffset = 0, textYOffset = 0;

    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
	    menuPtr->activeBorderWidthPtr, &activeBorderWidth);
    leftEdge = x + indicatorSpace + activeBorderWidth;

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
	     * The standard image position on Windows is in the indicator
	     * space to the left of the entries, unless this entry is a
	     * radio|check button because then the indicator space will be
	     * used.
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
    } else if (mePtr->bitmapPtr != NULL) {
	Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin, mePtr->bitmapPtr);
    	XCopyPlane(menuPtr->display, bitmap, d,	gc, 0, 0,
		(unsigned) imageWidth, (unsigned) imageHeight,
		leftEdge + imageXOffset,
		(int) (y + (mePtr->height - imageHeight)/2 + imageYOffset), 1);
    }
    if ((mePtr->compound != COMPOUND_NONE) || !haveImage) {
    	if (mePtr->labelLength > 0) {
	    int baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;
	    const char *label = Tcl_GetString(mePtr->labelPtr);

	    if (TkWinGetPlatformTheme() == TK_THEME_WIN_CLASSIC) {
		/*
		 * Win 95/98 systems draw disabled menu text with a 3D
		 * highlight, unless the menu item is highlighted,
		 */

		if ((mePtr->state == ENTRY_DISABLED) &&
			!(mePtr->entryFlags & ENTRY_PLATFORM_FLAG1)) {
		    COLORREF oldFgColor = gc->foreground;

		    gc->foreground = GetSysColor(COLOR_3DHILIGHT);
		    Tk_DrawChars(menuPtr->display, d, gc, tkfont, label,
			    mePtr->labelLength, leftEdge + textXOffset + 1,
			    baseline + textYOffset + 1);
		    gc->foreground = oldFgColor;
		}
	    }
	    Tk_DrawChars(menuPtr->display, d, gc, tkfont, label,
		    mePtr->labelLength, leftEdge + textXOffset,
		    baseline + textYOffset);
	    if (underline) {
		DrawMenuUnderline(menuPtr, mePtr, d, gc, tkfont, fmPtr,
			x + textXOffset, y + textYOffset, width, height);
	    }
	}
    }

    if (mePtr->state == ENTRY_DISABLED) {
	if (menuPtr->disabledFgPtr == NULL) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledGC, x, y,
		    (unsigned) width, (unsigned) height);
	} else if ((mePtr->image != NULL)
		&& menuPtr->disabledImageGC) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledImageGC,
		    leftEdge + imageXOffset,
		    (int) (y + (mePtr->height - imageHeight)/2 + imageYOffset),
		    (unsigned) imageWidth, (unsigned) imageHeight);
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpComputeMenubarGeometry --
 *
 *	This function is invoked to recompute the size and layout of a menu
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
    TkpComputeStandardMenuGeometry(menuPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawTearoffEntry --
 *
 *	This function draws the background part of a menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its current mode.
 *
 *----------------------------------------------------------------------
 */

void
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
	points[0].x += 2*segmentWidth;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureMenuEntry --
 *
 *	Processes configurations for menu entries.
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
    ScheduleMenuReconfigure(mePtr->menuPtr);
    return TCL_OK;
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
    Drawable menuDrawable,	/* Menu to draw into */
    Tk_Font tkfont,		/* Precalculated font for menu */
    const Tk_FontMetrics *menuMetricsPtr,
				/* Precalculated metrics for menu */
    int x,			/* X-coordinate of topleft of entry */
    int y,			/* Y-coordinate of topleft of entry */
    int width,			/* Width of the entry rectangle */
    int height,			/* Height of the current rectangle */
    int strictMotif,		/* Boolean flag */
    int drawingParameters)	/* Whether or not to draw the cascade arrow
				 * for cascade items and accelerator
				 * cues. */
{
    GC gc, indicatorGC;
    TkMenu *menuPtr = mePtr->menuPtr;
    Tk_3DBorder bgBorder, activeBorder;
    const Tk_FontMetrics *fmPtr;
    Tk_FontMetrics entryMetrics;
    int padY = (menuPtr->menuType == MENUBAR) ? 3 : 0;
    int adjustedX, adjustedY;
    int adjustedHeight = height - 2 * padY;
    TkWinDrawable memWinDraw;
    TkWinDCState dcState;
    HBITMAP oldBitmap = NULL;
    Drawable d;
    HDC memDc = NULL, menuDc = NULL;

    /*
     * If the menu entry includes an image then draw the entry into a
     * compatible bitmap first.  This avoids problems with clipping on
     * animated menus.  [Bug 1329198]
     */

    if (mePtr->image != NULL) {
	menuDc = TkWinGetDrawableDC(menuPtr->display, menuDrawable, &dcState);

	memDc = CreateCompatibleDC(menuDc);
	oldBitmap = SelectObject(memDc,
    			CreateCompatibleBitmap(menuDc, width, height) );

	memWinDraw.type = TWD_WINDC;
	memWinDraw.winDC.hdc = memDc;
	d = (Drawable)&memWinDraw;
	adjustedX = 0;
	adjustedY = padY;

    } else {
	d = menuDrawable;
	adjustedX = x;
	adjustedY = y + padY;
    }

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
    	const char *name;

    	for (cascadeEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
    		cascadeEntryPtr != NULL;
    		cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
	    name = Tcl_GetString(cascadeEntryPtr->namePtr);
    	    if (strcmp(name, Tk_PathName(menuPtr->tkwin)) == 0) {
    	    	if (mePtr->state == ENTRY_DISABLED) {
    	    	    parentDisabled = 1;
    	    	}
    	    	break;
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

    bgBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->borderPtr == NULL) ? menuPtr->borderPtr
	    : mePtr->borderPtr);
    if (strictMotif) {
	activeBorder = bgBorder;
    } else {
	activeBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->activeBorderPtr == NULL) ? menuPtr->activeBorderPtr
	    : mePtr->activeBorderPtr);
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
	    bgBorder, adjustedX, adjustedY-padY, width, height);

    if (mePtr->type == SEPARATOR_ENTRY) {
	DrawMenuSeparator(menuPtr, mePtr, d, gc, tkfont,
		fmPtr, adjustedX, adjustedY, width, adjustedHeight);
    } else if (mePtr->type == TEAROFF_ENTRY) {
	DrawTearoffEntry(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		adjustedX, adjustedY, width, adjustedHeight);
    } else {
	DrawMenuEntryLabel(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		adjustedX, adjustedY, width, adjustedHeight,
		(drawingParameters & DRAW_MENU_ENTRY_NOUNDERLINE)?0:1);
	DrawMenuEntryAccelerator(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		activeBorder, adjustedX, adjustedY, width, adjustedHeight);
	DrawMenuEntryArrow(menuPtr, mePtr, d, gc,
		activeBorder, adjustedX, adjustedY, width, adjustedHeight,
		(drawingParameters & DRAW_MENU_ENTRY_ARROW)?1:0);
	if (!mePtr->hideMargin) {
	    DrawMenuEntryIndicator(menuPtr, mePtr, d, gc, indicatorGC, tkfont,
		    fmPtr, adjustedX, adjustedY, width, adjustedHeight);
	}
    }

    /*
     * Copy the entry contents from the temporary bitmap to the menu.
     */

    if (mePtr->image != NULL) {
	BitBlt(menuDc, x, y, width, height, memDc, 0, 0, SRCCOPY);
	DeleteObject(SelectObject(memDc, oldBitmap));
	DeleteDC(memDc);

	TkWinReleaseDrawableDC(menuDrawable, menuDc, &dcState);
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
	 * Either it is compound or we don't have an image,
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
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryBackground --
 *
 *	This function draws the background part of a menu.
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
    TkMenu *menuPtr,		/* The menu we are drawing. */
    TkMenuEntry *mePtr,		/* The entry we are drawing. */
    Drawable d,			/* What we are drawing into */
    Tk_3DBorder activeBorder,	/* Border for active items */
    Tk_3DBorder bgBorder,	/* Border for the background */
    int x,			/* left edge */
    int y,			/* top edge */
    int width,			/* width of rectangle to draw */
    int height)			/* height of rectangle to draw */
{
    if (mePtr->state == ENTRY_ACTIVE
		|| (mePtr->entryFlags & ENTRY_PLATFORM_FLAG1)!=0 ) {
	bgBorder = activeBorder;
    }
    Tk_Fill3DRectangle(menuPtr->tkwin, d, bgBorder, x, y, width, height, 0,
	    TK_RELIEF_FLAT);
}

/*
 *--------------------------------------------------------------
 *
 * TkpComputeStandardMenuGeometry --
 *
 *	This function is invoked to recompute the size and layout of a menu
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
    Tk_Font menuFont, tkfont;
    Tk_FontMetrics menuMetrics, entryMetrics, *fmPtr;
    int x, y, height, width, indicatorSpace, labelWidth, accelWidth;
    int windowWidth, windowHeight, accelSpace;
    int i, j, lastColumnBreak = 0;
    int activeBorderWidth, borderWidth;

    if (menuPtr->tkwin == NULL) {
	return;
    }

    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
	    menuPtr->borderWidthPtr, &borderWidth);
    x = y = borderWidth;
    indicatorSpace = labelWidth = accelWidth = 0;
    windowHeight = 0;

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
    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
	    menuPtr->activeBorderWidthPtr, &activeBorderWidth);

    for (i = 0; i < menuPtr->numEntries; i++) {
	if (menuPtr->entries[i]->fontPtr == NULL) {
	    tkfont = menuFont;
	    fmPtr = &menuMetrics;
	} else {
	    tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
		    menuPtr->entries[i]->fontPtr);
    	    Tk_GetFontMetrics(tkfont, &entryMetrics);
    	    fmPtr = &entryMetrics;
    	}
	if ((i > 0) && menuPtr->entries[i]->columnBreak) {
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
	    indicatorSpace = labelWidth = accelWidth = 0;
	    lastColumnBreak = i;
	    y = borderWidth;
	}

	if (menuPtr->entries[i]->type == SEPARATOR_ENTRY) {
	    GetMenuSeparatorGeometry(menuPtr, menuPtr->entries[i], tkfont,
	    	    fmPtr, &width, &height);
	    menuPtr->entries[i]->height = height;
	} else if (menuPtr->entries[i]->type == TEAROFF_ENTRY) {
	    GetTearoffEntryGeometry(menuPtr, menuPtr->entries[i], tkfont,
	    	    fmPtr, &width, &height);
	    menuPtr->entries[i]->height = height;
	} else {
	    /*
	     * For each entry, compute the height required by that particular
	     * entry, plus three widths: the width of the label, the width to
	     * allow for an indicator to be displayed to the left of the label
	     * (if any), and the width of the accelerator to be displayed to
	     * the right of the label (if any). These sizes depend, of course,
	     * on the type of the entry.
	     */

	    GetMenuLabelGeometry(menuPtr->entries[i], tkfont, fmPtr, &width,
	    	    &height);
	    menuPtr->entries[i]->height = height;
	    if (width > labelWidth) {
	    	labelWidth = width;
	    }

	    GetMenuAccelGeometry(menuPtr, menuPtr->entries[i], tkfont,
		    fmPtr, &width, &height);
	    if (height > menuPtr->entries[i]->height) {
	    	menuPtr->entries[i]->height = height;
	    }
	    if (width > accelWidth) {
	    	accelWidth = width;
	    }

	    GetMenuIndicatorGeometry(menuPtr, menuPtr->entries[i], tkfont,
	    	    fmPtr, &width, &height);
	    if (height > menuPtr->entries[i]->height) {
	    	menuPtr->entries[i]->height = height;
	    }
	    if (width > indicatorSpace) {
	    	indicatorSpace = width;
	    }

	    menuPtr->entries[i]->height += 2 * activeBorderWidth + 1;
    	}
	menuPtr->entries[i]->y = y;
	y += menuPtr->entries[i]->height;
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
 * MenuSelectEvent --
 *
 *	Generates a "MenuSelect" virtual event. This can be used to do
 *	context-sensitive menu help.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Places a virtual event on the event queue.
 *
 *----------------------------------------------------------------------
 */

static void
MenuSelectEvent(
    TkMenu *menuPtr)		/* the menu we have selected. */
{
    XVirtualEvent event;
    union {DWORD msgpos; POINTS point;} root;

    event.type = VirtualEvent;
    event.serial = menuPtr->display->request;
    event.send_event = 0;
    event.display = menuPtr->display;
    Tk_MakeWindowExist(menuPtr->tkwin);
    event.event = Tk_WindowId(menuPtr->tkwin);
    event.root = XRootWindow(menuPtr->display, 0);
    event.subwindow = None;
    event.time = TkpGetMS();

    root.msgpos = GetMessagePos();
    event.x_root = root.point.x;
    event.y_root = root.point.y;
    event.state = TkWinGetModifierState();
    event.same_screen = 1;
    event.name = Tk_GetUid("MenuSelect");
    event.user_data = NULL;
    Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNotifyToplevelCreate --
 *
 *	This routine reconfigures the menu and the clones indicated by
 *	menuName becuase a toplevel has been created and any system menus need
 *	to be created.
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
    TkMenuReferences *menuRefPtr;
    TkMenu *menuPtr;

    if ((menuName != NULL) && (menuName[0] != '\0')) {
	menuRefPtr = TkFindMenuReferences(interp, menuName);
	if ((menuRefPtr != NULL) && (menuRefPtr->menuPtr != NULL)) {
	    for (menuPtr = menuRefPtr->menuPtr->masterMenuPtr; menuPtr != NULL;
		    menuPtr = menuPtr->nextInstancePtr) {
		if (menuPtr->menuType == MENUBAR) {
		    ScheduleMenuReconfigure(menuPtr);
		}
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetMenuHWND --
 *
 *	This function returns the HWND of a hidden menu Window that processes
 *	messages of a popup menu. This hidden menu window is used to handle
 *	either a dynamic popup menu in the same process or a pull-down menu of
 *	an embedded window in a different process.
 *
 * Results:
 *	Returns the HWND of the hidden menu Window.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HWND
Tk_GetMenuHWND(
    Tk_Window tkwin)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    TkMenuInit();
    return tsdPtr->embeddedMenuHWND;
}

/*
 *----------------------------------------------------------------------
 *
 * MenuExitHandler --
 *
 *	Unregisters the class of utility windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Menus have to be reinitialized next time.
 *
 *----------------------------------------------------------------------
 */

static void
MenuExitHandler(
    ClientData clientData)	    /* Not used */
{
    UnregisterClassW(MENU_CLASS_NAME, Tk_GetHINSTANCE());
    UnregisterClassW(EMBEDDED_MENU_CLASS_NAME, Tk_GetHINSTANCE());
}

/*
 *----------------------------------------------------------------------
 *
 * MenuExitHandler --
 *
 *	Throws away the utility window needed for menus and delete hash
 *	tables.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Menus have to be reinitialized next time.
 *
 *----------------------------------------------------------------------
 */

static void
MenuThreadExitHandler(
    ClientData clientData)	    /* Not used */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    DestroyWindow(tsdPtr->menuHWND);
    DestroyWindow(tsdPtr->embeddedMenuHWND);
    tsdPtr->menuHWND = NULL;
    tsdPtr->embeddedMenuHWND = NULL;

    Tcl_DeleteHashTable(&tsdPtr->winMenuTable);
    Tcl_DeleteHashTable(&tsdPtr->commandTable);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetMenuSystemDefault --
 *
 *	Gets the Windows specific default value for a given X resource
 *	database name.
 *
 * Results:
 *	Returns a Tcl_Obj* with the default value. If there is no
 *	Windows-specific default for this attribute, returns NULL. This object
 *	has a ref count of 0.
 *
 * Side effects:
 *	Storage is allocated.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkWinGetMenuSystemDefault(
    Tk_Window tkwin,		/* A window to use. */
    const char *dbName,		/* The option database name. */
    const char *className)	/* The name of the option class. */
{
    Tcl_Obj *valuePtr = NULL;

    if ((strcmp(dbName, "activeBorderWidth") == 0) ||
	    (strcmp(dbName, "borderWidth") == 0)) {
	valuePtr = Tcl_NewIntObj(defaultBorderWidth);
    } else if (strcmp(dbName, "font") == 0) {
	valuePtr = Tcl_NewStringObj(Tcl_DStringValue(&menuFontDString), -1);
    }

    return valuePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * SetDefaults --
 *
 *	Read system menu settings (font, sizes of items, use of accelerators)
 *	This is called if the UI theme or settings are changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May result in menu items being redrawn with different appearance.
 *
 *----------------------------------------------------------------------
 */

static void
SetDefaults(
    int firstTime)		/* Is this the first time this has been
				 * called? */
{
    char sizeString[TCL_INTEGER_SPACE];
    char faceName[LF_FACESIZE];
    HDC scratchDC;
    int bold = 0;
    int italic = 0;
    TEXTMETRICW tm;
    int pointSize;
    HFONT menuFont;
    /* See: [Bug #3239768] tk8.4.19 (and later) WIN32 menu font support */
    struct {
        NONCLIENTMETRICSW metrics;
#if (WINVER < 0x0600)
        int padding;
#endif
    } nc;
    OSVERSIONINFOW os;

    /*
     * Set all of the default options. The loop will terminate when we run out
     * of options via a break statement.
     */

    defaultBorderWidth = GetSystemMetrics(SM_CXBORDER);
    if (GetSystemMetrics(SM_CYBORDER) > defaultBorderWidth) {
	defaultBorderWidth = GetSystemMetrics(SM_CYBORDER);
    }

    scratchDC = CreateDCW(L"DISPLAY", NULL, NULL, NULL);
    if (!firstTime) {
	Tcl_DStringFree(&menuFontDString);
    }
    Tcl_DStringInit(&menuFontDString);

    nc.metrics.cbSize = sizeof(nc);

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    GetVersionExW(&os);
    if (os.dwMajorVersion < 6) {
	nc.metrics.cbSize -= sizeof(int);
    }

    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, nc.metrics.cbSize,
	    &nc.metrics, 0);
    menuFont = CreateFontIndirectW(&nc.metrics.lfMenuFont);
    SelectObject(scratchDC, menuFont);
    GetTextMetricsW(scratchDC, &tm);
    GetTextFaceA(scratchDC, LF_FACESIZE, faceName);
    pointSize = MulDiv(tm.tmHeight - tm.tmInternalLeading,
	    72, GetDeviceCaps(scratchDC, LOGPIXELSY));
    if (tm.tmWeight >= 700) {
	bold = 1;
    }
    if (tm.tmItalic) {
	italic = 1;
    }

    SelectObject(scratchDC, GetStockObject(SYSTEM_FONT));
    DeleteDC(scratchDC);

    DeleteObject(menuFont);

    Tcl_DStringAppendElement(&menuFontDString, faceName);
    sprintf(sizeString, "%d", pointSize);
    Tcl_DStringAppendElement(&menuFontDString, sizeString);

    if (bold || italic) {
	Tcl_DString boldItalicDString;

	Tcl_DStringInit(&boldItalicDString);
	if (bold) {
	    Tcl_DStringAppendElement(&boldItalicDString, "bold");
	}
	if (italic) {
	    Tcl_DStringAppendElement(&boldItalicDString, "italic");
	}
	Tcl_DStringAppendElement(&menuFontDString,
		Tcl_DStringValue(&boldItalicDString));
	Tcl_DStringFree(&boldItalicDString);
    }

    /*
     * Now we go ahead and get the dimensions of the check mark and the
     * appropriate margins. Since this is fairly hairy, we do it here to save
     * time when traversing large sets of menu items.
     *
     * The code below was given to me by Microsoft over the phone. It is the
     * only way to ensure menu items line up, and is not documented.
     * How strange the calculation of indicatorDimensions[1] is...!
     */

    indicatorDimensions[0] = GetSystemMetrics(SM_CYMENUCHECK);
    indicatorDimensions[1] = ((GetSystemMetrics(SM_CXFIXEDFRAME) +
	    GetSystemMetrics(SM_CXBORDER)
	    + GetSystemMetrics(SM_CXMENUCHECK) + 7) & 0xFFF8)
	    - GetSystemMetrics(SM_CXFIXEDFRAME);

    /*
     * Accelerators used to be always underlines until Win2K when a system
     * parameter was introduced to hide them unless Alt is pressed.
     */

    showMenuAccelerators = TRUE;
    SystemParametersInfoW(SPI_GETKEYBOARDCUES, 0, &showMenuAccelerators, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuInit --
 *
 *	Sets up the process-wide variables used by the menu package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	lastMenuID gets initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuInit(void)
{
    WNDCLASSW wndClass;

    wndClass.style = CS_OWNDC;
    wndClass.lpfnWndProc = TkWinMenuProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = Tk_GetHINSTANCE();
    wndClass.hIcon = NULL;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = MENU_CLASS_NAME;
    if (!RegisterClassW(&wndClass)) {
	Tcl_Panic("Failed to register menu window class");
    }

    wndClass.lpfnWndProc = TkWinEmbeddedMenuProc;
    wndClass.lpszClassName = EMBEDDED_MENU_CLASS_NAME;
    if (!RegisterClassW(&wndClass)) {
	Tcl_Panic("Failed to register embedded menu window class");
    }

    TkCreateExitHandler(MenuExitHandler, NULL);
    SetDefaults(1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuThreadInit --
 *
 *	Sets up the thread-local hash tables used by the menu module. Assumes
 *	that TkpMenuInit has been called.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hash tables winMenuTable and commandTable are initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuThreadInit(void)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->menuHWND = CreateWindowW(MENU_CLASS_NAME, L"MenuWindow", WS_POPUP,
	    0, 0, 10, 10, NULL, NULL, Tk_GetHINSTANCE(), NULL);

    if (!tsdPtr->menuHWND) {
	Tcl_Panic("Failed to create the menu window");
    }

    tsdPtr->embeddedMenuHWND =
	    CreateWindowW(EMBEDDED_MENU_CLASS_NAME, L"EmbeddedMenuWindow",
	    WS_POPUP, 0, 0, 10, 10, NULL, NULL, Tk_GetHINSTANCE(), NULL);

    if (!tsdPtr->embeddedMenuHWND) {
	Tcl_Panic("Failed to create the embedded menu window");
    }

    Tcl_InitHashTable(&tsdPtr->winMenuTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&tsdPtr->commandTable, TCL_ONE_WORD_KEYS);

    TkCreateThreadExitHandler(MenuThreadExitHandler, NULL);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
