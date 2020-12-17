/*
 * tkMacOSXMenu.c --
 *
 *	This module implements the Mac-platform specific features of menus.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2012 Adrian Robert.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkMenubutton.h"
#include "tkMenu.h"
#include "tkColor.h"
#include "tkFont.h"
#include "tkMacOSXWm.h"
#include "tkMacOSXDebug.h"
#include "tkMacOSXConstants.h"

/*
#ifdef TK_MAC_DEBUG
#define TK_MAC_DEBUG_MENUS
#endif
*/

#define ENTRY_HELP_MENU		ENTRY_PLATFORM_FLAG1
#define ENTRY_APPLE_MENU	ENTRY_PLATFORM_FLAG2
#define ENTRY_WINDOWS_MENU	ENTRY_PLATFORM_FLAG3

#define sl(s) ((int) (sizeof(s "") - 1))

#define SPECIALMENU(n, f) {.name = "." #n, .len = sl(#n) + 1, \
	.flag = ENTRY_##f##_MENU }
static const struct {
    const char *name; const size_t len; const int flag;
} specialMenus[] = {
    SPECIALMENU(help,	HELP),
    SPECIALMENU(apple,	APPLE),
    SPECIALMENU(window,	WINDOWS),
    {NULL}
};
#undef SPECIALMENU

#define MODIFIER(n, f) {.name = #n, .len = sl(#n), .mask = f }
static const struct {
    const char *name; const size_t len; const NSUInteger mask;
} modifiers[] = {
    MODIFIER(Control,	NSControlKeyMask),
    MODIFIER(Ctrl,	NSControlKeyMask),
    MODIFIER(Option,	NSAlternateKeyMask),
    MODIFIER(Opt,	NSAlternateKeyMask),
    MODIFIER(Alt,	NSAlternateKeyMask),
    MODIFIER(Shift,	NSShiftKeyMask),
    MODIFIER(Command,	NSCommandKeyMask),
    MODIFIER(Cmd,	NSCommandKeyMask),
    MODIFIER(Meta,	NSCommandKeyMask),
    {NULL}
};
#undef MODIFIER

#define ACCEL(n, c) {.name = #n, .len = sl(#n), .ch = c }
static const struct {
    const char *name; const size_t len; const UniChar ch;
} specialAccelerators[] = {
    ACCEL(PageUp,	NSPageUpFunctionKey),
    ACCEL(PageDown,	NSPageDownFunctionKey),
    ACCEL(Left,		NSLeftArrowFunctionKey),
    ACCEL(Right,	NSRightArrowFunctionKey),
    ACCEL(Up,		NSUpArrowFunctionKey),
    ACCEL(Down,		NSDownArrowFunctionKey),
    ACCEL(Escape,	0x001b),
    ACCEL(Clear,	NSClearDisplayFunctionKey),
    ACCEL(Enter,	NSEnterCharacter),
    ACCEL(Backspace,	NSBackspaceCharacter),
    ACCEL(Space,	' '),
    ACCEL(Tab,		NSTabCharacter),
    ACCEL(BackTab,	NSBackTabCharacter),
    ACCEL(Delete,	NSDeleteCharacter),
    ACCEL(Home,		NSHomeFunctionKey),
    ACCEL(End,		NSEndFunctionKey),
    ACCEL(Return,	NSCarriageReturnCharacter),
    ACCEL(Help,		NSHelpFunctionKey),
    ACCEL(Power,	0x233d),
    ACCEL(Eject,	0xf804),
    {NULL}
};
#undef ACCEL
#undef sl

static int gNoTkMenus = 0;	/* This is used by Tk_MacOSXTurnOffMenus as
				 * the flag that Tk is not to draw any
				 * menus. */
static int inPostMenu = 0;
static unsigned long defaultBg = 0, defaultFg = 0;
static SInt32 menuMarkColumnWidth = 0, menuIconTrailingEdgeMargin = 0;
static SInt32 menuTextLeadingEdgeMargin = 0, menuTextTrailingEdgeMargin = 0;
static SInt16 menuItemExtraHeight = 0, menuItemExtraWidth = 0;
static SInt16 menuSeparatorHeight = 0;

static void	CheckForSpecialMenu(TkMenu *menuPtr);
static NSString *ParseAccelerator(const char *accel, NSUInteger *maskPtr);
static int	GenerateMenuSelectEvent(TKMenu *menu, NSMenuItem *menuItem);
static void	MenuSelectEvent(TkMenu *menuPtr);
static void	RecursivelyClearActiveMenu(TkMenu *menuPtr);
static int	ModifierCharWidth(Tk_Font tkfont);

#pragma mark TKMenu

@interface TKMenu(TKMenuPrivate)
- (id) initWithTkMenu: (TkMenu *) tkMenu;
- (TkMenu *) tkMenu;
- (int) tkIndexOfItem: (NSMenuItem *) menuItem;
- (void) insertItem: (NSMenuItem *) newItem atTkIndex: (NSInteger) index;
@end

#define TKMenu_NSMenuDelegate <NSMenuDelegate>
@interface TKMenu(TKMenuDelegate) TKMenu_NSMenuDelegate
@end

@implementation TKMenu
- (void) setSpecial: (NSUInteger) special
{
    NSAssert(!_tkSpecial, @"Cannot change specialness of a special menu");
    _tkSpecial = special;
}
- (BOOL) isSpecial: (NSUInteger) special
{
    return (_tkSpecial == special);
}
@end

@implementation TKMenu(TKMenuPrivate)

- (id) initWithTitle: (NSString *) aTitle
{
    self = [super initWithTitle:aTitle];
    if (self) {
	_tkMenu = NULL;
	_tkOffset = 0;
	_tkItemCount = 0;
	_tkSpecial = 0;
	[self setDelegate:self];
    }
    return self;
}

- (id) initWithTkMenu: (TkMenu *) tkMenu
{
    NSString *title = [[NSString alloc] initWithUTF8String:
	    Tk_PathName(tkMenu->tkwin)];

    self = [self initWithTitle:title];
    [title release];
    if (self) {
	_tkMenu = tkMenu;
    }
    return self;
}

- (id) copyWithZone: (NSZone *) zone
{
    TKMenu *copy = [super copyWithZone:zone];

    NSAssert(_tkMenu == nil, @"Cannot copy tkMenu");
    copy->_tkMenu = _tkMenu;
    copy->_tkOffset = _tkOffset;
    copy->_tkItemCount = _tkItemCount;
    copy->_tkSpecial = _tkSpecial;
    return copy;
}

- (TkMenu *) tkMenu
{
    return _tkMenu;
}

- (int) tkIndexOfItem: (NSMenuItem *) menuItem
{
    return [self indexOfItem:menuItem] - _tkOffset;
}

- (void) insertItem: (NSMenuItem *) newItem atTkIndex: (NSInteger) index
{
    [super insertItem:newItem atIndex:index + _tkOffset];
    _tkItemCount++;
}

- (void) insertItem: (NSMenuItem *) newItem atIndex: (NSInteger) index
{
    if (_tkMenu && index >= 0) {
	if ((NSUInteger)index <= _tkOffset) {
	    _tkOffset++;
	} else {
	    NSAssert((NSUInteger)index >= _tkItemCount + _tkOffset,
		    @"Cannot insert in the middle of Tk menu");
	}
    }
    [super insertItem:newItem atIndex:index];
}

- (void) removeItemAtIndex: (NSInteger) index
{
    if (_tkMenu && index >= 0) {
	if ((NSUInteger)index < _tkOffset) {
	    _tkOffset--;
	} else if ((NSUInteger)index < _tkItemCount + _tkOffset) {
	    _tkItemCount--;
	}
    }
    [super removeItemAtIndex:index];
}

- (NSMenuItem *) newTkMenuItem: (TkMenuEntry *) mePtr
{
    NSMenuItem *menuItem = [[NSMenuItem alloc] initWithTitle:@""
	    action:@selector(tkMenuItemInvoke:) keyEquivalent:@""];

    [menuItem setTarget:self];
    [menuItem setTag:(NSInteger)mePtr];
    return menuItem;
}
@end

@implementation TKMenu(TKMenuActions)
// target methods

- (BOOL) validateMenuItem: (NSMenuItem *) menuItem
{
    return [menuItem isEnabled];
}

// Workaround for bug 3572016; leaves menu items enabled during modal dialog.
- (BOOL)worksWhenModal
{
    return YES;
}

- (void) tkMenuItemInvoke: (id) sender
{
    /*
     * With the delegate matching key equivalents, when a menu action is sent
     * in response to a key equivalent, sender is the whole menu and not the
     * the specific menu item, use this to ignore key equivalents for our
     * menus (as Tk handles them directly via bindings).
     */

    if ([sender isKindOfClass:[NSMenuItem class]]) {
	NSMenuItem *menuItem = (NSMenuItem *)sender;
	TkMenu *menuPtr = (TkMenu *)_tkMenu;
	TkMenuEntry *mePtr = (TkMenuEntry *)[menuItem tag];

	if (menuPtr && mePtr) {
	    Tcl_Interp *interp = menuPtr->interp;
	    /*Add time for errors to fire if necessary. This is sub-optimal
	     *but avoids issues with Tcl/Cocoa event loop integration.
	     */
	    Tcl_Sleep(100);
	    Tcl_Preserve(interp);
	    Tcl_Preserve(menuPtr);

	    int result = TkInvokeMenu(interp, menuPtr, mePtr->index);

	    if (result != TCL_OK && result != TCL_CONTINUE &&
		    result != TCL_BREAK) {
		Tcl_AddErrorInfo(interp, "\n    (menu invoke)");
		Tcl_BackgroundException(interp, result);
	    }
	    Tcl_Release(menuPtr);
	    Tcl_Release(interp);
	}
    }
}
@end

@implementation TKMenu(TKMenuDelegate)
#define keyEquivModifiersMatch(km, m) (( \
    ((km) & NSCommandKeyMask) != ((m) & NSCommandKeyMask) || \
    ((km) & NSAlternateKeyMask) != ((m) & NSAlternateKeyMask) || \
    ((km) & NSControlKeyMask) != ((m) & NSControlKeyMask) || \
    (((km) & NSShiftKeyMask) != ((m) & NSShiftKeyMask) && \
    ((m) & NSFunctionKeyMask))) ? NO : YES)

- (BOOL) menuHasKeyEquivalent: (NSMenu *) menu forEvent: (NSEvent *) event
	target: (id *) target action: (SEL *) action
{
    /*Use lowercaseString to keep "shift" from firing twice if bound to different procedure.*/
    NSString *key = [[event charactersIgnoringModifiers] lowercaseString];
    NSUInteger modifiers = [event modifierFlags] &
	    NSDeviceIndependentModifierFlagsMask;

    if (modifiers == (NSCommandKeyMask | NSShiftKeyMask) &&
	    [key compare:@"?"] == NSOrderedSame) {
	return NO;
    }

    // For command key, take input manager's word so things
    // like dvorak / qwerty layout work.
    if (([event modifierFlags] & NSCommandKeyMask) == NSCommandKeyMask) {
      key = [event characters];
    }

    NSArray *itemArray = [self itemArray];

    for (NSMenuItem *item in itemArray) {
	if ([item isEnabled] && [[item keyEquivalent] compare:key] ==
		NSOrderedSame) {
	    NSUInteger keyEquivModifiers = [item keyEquivalentModifierMask];

	    if (keyEquivModifiersMatch(keyEquivModifiers, modifiers)) {
		*target = [item target];
		*action = [item action];
		return YES;
	    }
	}
    }
    return NO;
}

- (void) menuWillOpen: (NSMenu *) menu
{
    if (_tkMenu) {
	//RecursivelyClearActiveMenu(_tkMenu);
	GenerateMenuSelectEvent((TKMenu *)[self supermenu],
		[self itemInSupermenu]);
    }
}

- (void) menuDidClose: (NSMenu *) menu
{
    if (_tkMenu) {
	RecursivelyClearActiveMenu(_tkMenu);
    }
}

- (void) menu: (NSMenu *) menu willHighlightItem: (NSMenuItem *) item
{
    if (_tkMenu) {
	GenerateMenuSelectEvent(self, item);
    }
}

- (void) menuNeedsUpdate: (NSMenu *) menu
{
    TkMenu *menuPtr = (TkMenu *) _tkMenu;

    if (menuPtr) {
	Tcl_Interp *interp = menuPtr->interp;

	Tcl_Preserve(interp);
	Tcl_Preserve(menuPtr);

	int result = TkPostCommand(_tkMenu);

	if (result!=TCL_OK && result!=TCL_CONTINUE && result!=TCL_BREAK) {
	      Tcl_AddErrorInfo(interp, "\n    (menu preprocess)");
	      Tcl_BackgroundException(interp, result);
	}
	Tcl_Release(menuPtr);
	Tcl_Release(interp);
    }
}
@end

#pragma mark TKApplication(TKMenu)

@interface NSApplication(TKMenu)
- (void) setAppleMenu: (NSMenu *) menu;
@end

@implementation TKApplication(TKMenu)

- (void) menuBeginTracking: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    //TkMacOSXClearMenubarActive();
    //TkMacOSXPreprocessMenu();
}

- (void) menuEndTracking: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    if (!inPostMenu) {
	TkMacOSXClearMenubarActive();
    }
}

- (void) tkSetMainMenu: (TKMenu *) menu
{
    if (gNoTkMenus) {
	return;
    }

    TKMenu *applicationMenu = nil;

    if (menu) {
	NSMenuItem *applicationMenuItem = [menu numberOfItems] ?
		[menu itemAtIndex:0] : nil;

	if (![menu isSpecial:tkMainMenu]) {
	    TkMenuEntry *mePtr = (TkMenuEntry *)[applicationMenuItem tag];

	    if (!mePtr || !(mePtr->entryFlags & ENTRY_APPLE_MENU)) {
		applicationMenuItem = [NSMenuItem itemWithSubmenu:
			[_defaultApplicationMenu copy]];
		[menu insertItem:applicationMenuItem atIndex:0];
	    }
	    [menu setSpecial:tkMainMenu];
	}
	applicationMenu = (TKMenu *)[applicationMenuItem submenu];
	if (![applicationMenu isSpecial:tkApplicationMenu]) {
	    for (NSMenuItem *item in _defaultApplicationMenuItems) {
		[applicationMenu addItem:[item copy]];
	    }
	    [applicationMenu setSpecial:tkApplicationMenu];
	}

	NSArray *itemArray = [menu itemArray];

	for (NSMenuItem *item in itemArray) {
	    TkMenuEntry *mePtr = (TkMenuEntry *)[item tag];
	    TKMenu *submenu = (TKMenu *)[item submenu];
	    if (mePtr && submenu) {
		if ((mePtr->entryFlags & ENTRY_WINDOWS_MENU) &&
			![submenu isSpecial:tkWindowsMenu]) {
		    NSInteger index = 0;

		    for (NSMenuItem *i in _defaultWindowsMenuItems) {
			[submenu insertItem:[i copy] atIndex:index++];
		    }
		    [self setWindowsMenu:submenu];
		    [submenu setSpecial:tkWindowsMenu];
		} else if ((mePtr->entryFlags & ENTRY_HELP_MENU) &&
			![submenu isSpecial:tkHelpMenu]) {
		    NSInteger index = 0;

		    for (NSMenuItem *i in _defaultHelpMenuItems) {
			[submenu insertItem:[i copy] atIndex:index++];
		    }
		    [submenu setSpecial:tkHelpMenu];
		}
	    }
	}
    } else {
	menu = _defaultMainMenu;
	applicationMenu = _defaultApplicationMenu;
    }

    NSMenuItem *servicesMenuItem =
	    [applicationMenu itemWithTitle:@"Services"];

    if (servicesMenuItem && [servicesMenuItem submenu] != _servicesMenu) {
	[[_servicesMenu itemInSupermenu] setSubmenu:nil];
	[servicesMenuItem setSubmenu:_servicesMenu];
    }
    [self setAppleMenu:applicationMenu];
    [self setMainMenu:menu];
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * TkpNewMenu --
 *
 *	Gets a new blank menu. Only the platform specific options are filled
 *	in.
 *
 * Results:
 *	Returns a standard Tcl error.
 *
 * Side effects:
 *	Allocates a NSMenu and puts it into the platformData field of the
 *	menuPtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpNewMenu(
    TkMenu *menuPtr)		/* The common structure we are making the
				 * platform structure for. */
{
    TKMenu *menu = [[TKMenu alloc] initWithTkMenu:menuPtr];
    menuPtr->platformData = (TkMenuPlatformData) menu;
    CheckForSpecialMenu(menuPtr);
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
    NSMenu* nsmenu = (NSMenu*)(menuPtr->platformData);

    [nsmenu release];
    menuPtr->platformData = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNewEntry --
 *
 *	Adds a pointer to a new menu entry structure with the platform-
 *	specific fields filled in. The Macintosh uses the platformEntryData
 *	field of the TkMenuEntry record.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	Storage gets allocated. New menu entry data is put into the
 *	platformEntryData field of the mePtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpMenuNewEntry(
    TkMenuEntry *mePtr)		/* The menu we are adding an entry to */
{
    TKMenu *menu = (TKMenu *) mePtr->menuPtr->platformData;
    NSMenuItem *menuItem;
    if (mePtr->type == SEPARATOR_ENTRY || mePtr->type == TEAROFF_ENTRY) {
	menuItem = [[NSMenuItem separatorItem] retain];
    } else {
	menuItem = [menu newTkMenuItem:mePtr];
    }
    mePtr->platformEntryData = (TkMenuPlatformEntryData) menuItem;

    /*
     * Caller TkMenuEntry() already did this same insertion into the generic
     * TkMenu so we just match it for the platform menu.
     */

    [menu insertItem:menuItem atTkIndex:mePtr->index];
    return TCL_OK;
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
    TkMenuEntry *mePtr) 	/* Information about menu entry; may or may
				 * not already have values for some fields. */
{
    NSMenuItem *menuItem = (NSMenuItem *) mePtr->platformEntryData;
    NSString *title = @"";
    NSAttributedString *attributedTitle = nil;
    NSImage *image = nil;
    NSString *keyEquivalent = @"";
    NSUInteger modifierMask = NSCommandKeyMask;
    NSMenu *submenu = nil;
    NSDictionary *attributes;
    int imageWidth, imageHeight;
    GC gc = (mePtr->textGC ? mePtr->textGC : mePtr->menuPtr->textGC);
    Tcl_Obj *fontPtr = (mePtr->fontPtr ? mePtr->fontPtr :
	    mePtr->menuPtr->fontPtr);

    if (mePtr->image) {
    	Tk_SizeOfImage(mePtr->image, &imageWidth, &imageHeight);
	image = TkMacOSXGetNSImageWithTkImage(mePtr->menuPtr->display,
		mePtr->image, imageWidth, imageHeight);
    } else if (mePtr->bitmapPtr != None) {
	Pixmap bitmap = Tk_GetBitmapFromObj(mePtr->menuPtr->tkwin,
		mePtr->bitmapPtr);

	Tk_SizeOfBitmap(mePtr->menuPtr->display, bitmap, &imageWidth,
		&imageHeight);
	image = TkMacOSXGetNSImageWithBitmap(mePtr->menuPtr->display, bitmap,
		gc, imageWidth, imageHeight);
    }
    [menuItem setImage:image];
    if ((!image || mePtr->compound != COMPOUND_NONE) && mePtr->labelPtr &&
	    mePtr->labelLength) {
	title = [[[NSString alloc] initWithBytes:Tcl_GetString(mePtr->labelPtr)
		length:mePtr->labelLength encoding:NSUTF8StringEncoding]
		autorelease];
	if ([title hasSuffix:@"..."]) {
	    title = [NSString stringWithFormat:@"%@%C",
		    [title substringToIndex:[title length] - 3], 0x2026];
	}
    }
    [menuItem setTitle:title];
    if (strcmp(Tcl_GetString(fontPtr), "menu") || gc->foreground != defaultFg
	    || gc->background != defaultBg) {
	attributes = TkMacOSXNSFontAttributesForFont(Tk_GetFontFromObj(
		mePtr->menuPtr->tkwin, fontPtr));
	if (gc->foreground != defaultFg || gc->background != defaultBg) {
	    NSColor *color = TkMacOSXGetNSColor(gc,
		    gc->foreground!=defaultFg? gc->foreground:gc->background);

	    attributes = [[attributes mutableCopy] autorelease];
	    [(NSMutableDictionary *)attributes setObject:color
		    forKey:NSForegroundColorAttributeName];
	}
	if (attributes) {
	    attributedTitle = [[[NSAttributedString alloc]
		    initWithString:title attributes:attributes] autorelease];
	}
    }
    [menuItem setAttributedTitle:attributedTitle];
    [menuItem setEnabled:!(mePtr->state == ENTRY_DISABLED)];
    [menuItem setState:((mePtr->type == CHECK_BUTTON_ENTRY ||
	    mePtr->type == RADIO_BUTTON_ENTRY) && mePtr->indicatorOn &&
	    (mePtr->entryFlags & ENTRY_SELECTED) ? NSOnState : NSOffState)];
    if (mePtr->type != CASCADE_ENTRY && mePtr->accelPtr && mePtr->accelLength) {
	keyEquivalent = ParseAccelerator(Tcl_GetString(mePtr->accelPtr),
		&modifierMask);
    }
    [menuItem setKeyEquivalent:keyEquivalent];
    [menuItem setKeyEquivalentModifierMask:modifierMask];
    if (mePtr->type == CASCADE_ENTRY && mePtr->namePtr) {
	TkMenuReferences *menuRefPtr;

	menuRefPtr = TkFindMenuReferencesObj(mePtr->menuPtr->interp,
		mePtr->namePtr);
	if (menuRefPtr && menuRefPtr->menuPtr) {
	    CheckForSpecialMenu(menuRefPtr->menuPtr);
	    submenu = (TKMenu *) menuRefPtr->menuPtr->platformData;
	    if ([submenu supermenu] && [menuItem submenu] != submenu) {
		/*
		 * This happens during a clone, where the parent menu is
		 * cloned before its children, so just ignore this temprary
		 * setting, it will be changed shortly (c.f. tkMenu.c
		 * CloneMenu())
		 */

		submenu = nil;
	    } else {
		[submenu setTitle:title];

    		if ([menuItem isEnabled]) {
		  /* This menuItem might have been previously disabled (XXX:
		     track this), which would have disabled entries; we must
		     re-enable the entries here. */
		  int i = 0;
		  NSArray *itemArray = [submenu itemArray];
		  for (NSMenuItem *item in itemArray) {
		    TkMenuEntry *submePtr = menuRefPtr->menuPtr->entries[i];
		    /* Work around an apparent bug where itemArray can have
                      more items than the menu's entries[] array. */
                    if (i >= menuRefPtr->menuPtr->numEntries) break;
		    [item setEnabled: !(submePtr->state == ENTRY_DISABLED)];
		    i++;
		  }
		}

	    }
	}
    }
    [menuItem setSubmenu:submenu];

    return TCL_OK;
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
    TkMenuEntry *mePtr)
{
    NSMenuItem *menuItem;
    TKMenu *menu;
    NSInteger index;

    if (mePtr->platformEntryData && mePtr->menuPtr->platformData) {
	menu = (TKMenu *) mePtr->menuPtr->platformData;
	menuItem = (NSMenuItem *) mePtr->platformEntryData;
	index = [menu indexOfItem:menuItem];

	if (index > -1) {
	    [menu removeItemAtIndex:index];
	}
	[menuItem release];
	mePtr->platformEntryData = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPostMenu --
 *
 *	Posts a menu on the screen
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
    Tcl_Interp *interp,		/* The interpreter this menu lives in */
    TkMenu *menuPtr,		/* The menu we are posting */
    int x,			/* The global x-coordinate of the top, left-
				 * hand corner of where the menu is supposed
				 * to be posted. */
    int y)			/* The global y-coordinate */
{


    /* Get the object that holds this Tk Window.*/
    Tk_Window root;
    root = Tk_MainWindow(interp);
    if (root == NULL) {
	return TCL_ERROR;
    }

    Drawable d = Tk_WindowId(root);
    NSView *rootview = TkMacOSXGetRootControl(d);
    NSWindow *win = [rootview window];
    int result;

    inPostMenu = 1;

    result = TkPreprocessMenu(menuPtr);
    if (result != TCL_OK) {
        inPostMenu = 0;
        return result;
    }

    int oldMode = Tcl_SetServiceMode(TCL_SERVICE_NONE);
    NSView *view = [win contentView];
    NSRect frame = NSMakeRect(x + 9, tkMacOSXZeroScreenHeight - y - 9, 1, 1);

    frame.origin = [view convertPoint:
	    [win tkConvertPointFromScreen:frame.origin] fromView:nil];

    NSMenu *menu = (NSMenu *) menuPtr->platformData;
    NSPopUpButtonCell *popUpButtonCell = [[NSPopUpButtonCell alloc]
	    initTextCell:@"" pullsDown:NO];

    [popUpButtonCell setAltersStateOfSelectedItem:NO];
    [popUpButtonCell setMenu:menu];
    [popUpButtonCell selectItem:nil];
    [popUpButtonCell performClickWithFrame:frame inView:view];
    [popUpButtonCell release];
    Tcl_SetServiceMode(oldMode);
    inPostMenu = 0;
    return TCL_OK;
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
 *	On Windows and UNIX, associates the platform menu with the platform
 *	window.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetWindowMenuBar(
    Tk_Window tkwin,		/* The window we are setting the menu in */
    TkMenu *menuPtr)		/* The menu we are setting */
{
    TkWindow *winPtr = (TkWindow *) tkwin;

    if (winPtr->wmInfoPtr) {
	winPtr->wmInfoPtr->menuPtr = menuPtr;
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
 *      This is a no-op on all other platforms.  On OS X it is a no-op when
 *      passed a NULL menuName or a nonexistent menuName, with an exception
 *      for the first call in a new interpreter.  In that special case, passing a
 *      NULL menuName installs the default menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menubar may be changed.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetMainMenubar(
    Tcl_Interp *interp,		/* The interpreter of the application */
    Tk_Window tkwin,		/* The frame we are setting up */
    const char *menuName)	/* The name of the menu to put in front. */
{
    static Tcl_Interp *currentInterp = NULL;
    TKMenu *menu = nil;

    if (menuName) {
	TkWindow *winPtr = (TkWindow *) tkwin;

	if (winPtr->wmInfoPtr && winPtr->wmInfoPtr->menuPtr &&
		winPtr->wmInfoPtr->menuPtr->masterMenuPtr &&
		winPtr->wmInfoPtr->menuPtr->masterMenuPtr->tkwin &&
		!strcmp(menuName, Tk_PathName(
		winPtr->wmInfoPtr->menuPtr->masterMenuPtr->tkwin))) {
	    menu = (TKMenu *) winPtr->wmInfoPtr->menuPtr->platformData;
	} else {
	    TkMenuReferences *menuRefPtr = TkFindMenuReferences(interp,
		    menuName);

	    if (menuRefPtr && menuRefPtr->menuPtr &&
		    menuRefPtr->menuPtr->platformData) {
		menu = (TKMenu *) menuRefPtr->menuPtr->platformData;
	    }
	}
    }
    if (menu || interp != currentInterp) {
	[NSApp tkSetMainMenu:menu];
    }
    currentInterp = interp;
}

/*
 *----------------------------------------------------------------------
 *
 * CheckForSpecialMenu --
 *
 *	Given a menu, check to see whether or not it is a cascade in a menubar
 *	with one of the special names .apple, .help or .window If it is, the
 *	entry that points to this menu will be marked.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Will set entryFlags appropriately.
 *
 *----------------------------------------------------------------------
 */

static void
CheckForSpecialMenu(
    TkMenu *menuPtr)		/* The menu we are checking */
{
    if (!menuPtr->masterMenuPtr->tkwin) {
	return;
    }
    for (TkMenuEntry *cascadeEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
	    cascadeEntryPtr;
	    cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
	if (cascadeEntryPtr->menuPtr->menuType == MENUBAR
		&& cascadeEntryPtr->menuPtr->masterMenuPtr->tkwin) {
	    TkMenu *masterMenuPtr = cascadeEntryPtr->menuPtr->masterMenuPtr;
	    int i = 0;
	    Tcl_DString ds;

	    Tcl_DStringInit(&ds);
	    Tcl_DStringAppend(&ds, Tk_PathName(masterMenuPtr->tkwin), -1);
	    while (specialMenus[i].name) {
		Tcl_DStringAppend(&ds, specialMenus[i].name,
			specialMenus[i].len);
		if (strcmp(Tcl_DStringValue(&ds),
			Tk_PathName(menuPtr->masterMenuPtr->tkwin)) == 0) {
		    cascadeEntryPtr->entryFlags |= specialMenus[i].flag;
		} else {
		    cascadeEntryPtr->entryFlags &= ~specialMenus[i].flag;
		}
		Tcl_DStringSetLength(&ds, Tcl_DStringLength(&ds) -
			specialMenus[i].len);
		i++;
	    }
	    Tcl_DStringFree(&ds);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ParseAccelerator --
 *
 *	Parse accelerator string.
 *
 * Results:
 *	Accelerator string & flags.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static NSString *
ParseAccelerator(
    const char *accel,
    NSUInteger *maskPtr)
{
    unichar ch = 0;
    size_t len;
    int i;

    *maskPtr = 0;
    while (1) {
	i = 0;
	while (modifiers[i].name) {
	    int l = modifiers[i].len;

	    if (!strncasecmp(accel, modifiers[i].name, l) &&
		    (accel[l] == '-' || accel[l] == '+')) {
		*maskPtr |= modifiers[i].mask;
		accel += l+1;
		break;
	    }
	    i++;
	}
	if (!modifiers[i].name || !*accel) {
	    break;
	}
    }
    len = strlen(accel);
    if (len > 1) {
	i = 0;
	if (accel[0] == 'F' && len < 4 && accel[1] > '0' && accel[1] <= '9') {
	    int fkey = accel[1] - '0';

	    if (len == 3) {
		if (accel[2] >= '0' && accel[2] <= '9') {
		    fkey = 10 * fkey + (accel[2] - '0');
		} else {
		    fkey = 0;
		}
	    }
	    if (fkey >= 1 && fkey <= 15) {
		ch = NSF1FunctionKey + fkey - 1;
	    }
	} else while (specialAccelerators[i].name) {
	    if (accel[0] == specialAccelerators[i].name[0] &&
		    len == specialAccelerators[i].len && !strncasecmp(accel,
		    specialAccelerators[i].name, specialAccelerators[i].len)) {
		ch = specialAccelerators[i].ch;
		break;
	    }
	    i++;
	}
    }
    if (ch) {
	return [[[NSString alloc] initWithCharacters:&ch length:1] autorelease];
    } else {
	return [[[[NSString alloc] initWithUTF8String:accel] autorelease]
		lowercaseString];
    }
}

/*
 *--------------------------------------------------------------
 *
 * ModifierCharWidth --
 *
 *	Helper mesuring width of command char in given font.
 *
 * Results:
 *	Width of command char.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
ModifierCharWidth(
    Tk_Font tkfont)
{
    static NSString *cmdChar = nil;

    if (!cmdChar) {
	unichar cmd = kCommandUnicode;

	cmdChar = [[NSString alloc] initWithCharacters:&cmd length:1];
    }
    return [cmdChar sizeWithAttributes:
	    TkMacOSXNSFontAttributesForFont(tkfont)].width;
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
    int modifierCharWidth, menuModifierCharWidth;
    int x, y, modifierWidth, labelWidth, indicatorSpace;
    int windowWidth, windowHeight, accelWidth;
    int i, j, lastColumnBreak, maxWidth;
    int entryWidth, maxIndicatorSpace, borderWidth, activeBorderWidth;
    TkMenuEntry *mePtr, *columnEntryPtr;
    int haveAccel = 0;

    if (menuPtr->tkwin == NULL) {
	return;
    }

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
	    &borderWidth);
    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
	    &activeBorderWidth);
    x = y = borderWidth;
    windowHeight = maxWidth = lastColumnBreak = 0;
    maxIndicatorSpace = 0;

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
    menuModifierCharWidth = ModifierCharWidth(menuFont);

    for (i = 0; i < menuPtr->numEntries; i++) {
	mePtr = menuPtr->entries[i];
	if (mePtr->type == CASCADE_ENTRY || mePtr->accelLength > 0) {
	    haveAccel = 1;
	    break;
	}
    }

    for (i = 0; i < menuPtr->numEntries; i++) {
	mePtr = menuPtr->entries[i];
	if (mePtr->fontPtr == NULL) {
	    tkfont = menuFont;
	    fmPtr = &menuMetrics;
	    modifierCharWidth = menuModifierCharWidth;
	} else {
	    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
	    Tk_GetFontMetrics(tkfont, &entryMetrics);
	    fmPtr = &entryMetrics;
	    modifierCharWidth = ModifierCharWidth(tkfont);
	}

	if ((i > 0) && mePtr->columnBreak) {
	    if (maxIndicatorSpace != 0) {
		maxIndicatorSpace += 2;
	    }
	    for (j = lastColumnBreak; j < i; j++) {
		columnEntryPtr = menuPtr->entries[j];
		columnEntryPtr->indicatorSpace = maxIndicatorSpace;
		columnEntryPtr->width = maxIndicatorSpace + maxWidth
			+ 2 * activeBorderWidth;
		columnEntryPtr->x = x;
		columnEntryPtr->entryFlags &= ~ENTRY_LAST_COLUMN;
	    }
	    x += maxIndicatorSpace + maxWidth + 2 * activeBorderWidth;
	    maxWidth = maxIndicatorSpace = 0;
	    lastColumnBreak = i;
	    y = borderWidth;
	}
	accelWidth = modifierWidth = indicatorSpace = 0;
	if (mePtr->type == SEPARATOR_ENTRY || mePtr->type == TEAROFF_ENTRY) {
	    mePtr->height = menuSeparatorHeight;
	} else {
	    /*
	     * For each entry, compute the height required by that particular
	     * entry, plus three widths: the width of the label, the width to
	     * allow for an indicator to be displayed to the left of the label
	     * (if any), and the width of the accelerator to be displayed to
	     * the right of the label (if any). These sizes depend, of course,
	     * on the type of the entry.
	     */

	    NSMenuItem *menuItem = (NSMenuItem *) mePtr->platformEntryData;
	    int haveImage = 0, width = 0, height = 0;

	    if (mePtr->image) {
		Tk_SizeOfImage(mePtr->image, &width, &height);
		haveImage = 1;
	    } else if (mePtr->bitmapPtr) {
		Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin,
			mePtr->bitmapPtr);

		Tk_SizeOfBitmap(menuPtr->display, bitmap, &width, &height);
		haveImage = 1;
	    }
	    if (!haveImage || (mePtr->compound != COMPOUND_NONE)) {
		NSAttributedString *attrTitle = [menuItem attributedTitle];
		NSSize size;

		if (attrTitle) {
		    size = [attrTitle size];
		} else {
		    size = [[menuItem title] sizeWithAttributes:
			TkMacOSXNSFontAttributesForFont(tkfont)];
		}
		size.width += menuTextLeadingEdgeMargin +
			menuTextTrailingEdgeMargin;
		if (size.height < fmPtr->linespace) {
		    size.height = fmPtr->linespace;
		}
		if (haveImage && (mePtr->compound != COMPOUND_NONE)) {
		    int margin = width + menuIconTrailingEdgeMargin;

		    if (margin > menuTextLeadingEdgeMargin) {
			margin = menuTextLeadingEdgeMargin;
		    }
		    width += size.width + menuIconTrailingEdgeMargin - margin;
		    if (size.height > height) {
			height = size.height;
		    }
		} else {
		    width = size.width;
		    height = size.height;
		}
	    }
	    labelWidth = width + menuItemExtraWidth;
	    mePtr->height = height + menuItemExtraHeight;

	    if (mePtr->type == CASCADE_ENTRY) {
		modifierWidth = modifierCharWidth;
	    } else if (mePtr->accelLength == 0) {
		if (haveAccel && !mePtr->hideMargin) {
		    modifierWidth = modifierCharWidth;
		}
	    } else {
		NSUInteger modifMask = [menuItem keyEquivalentModifierMask];
		int i = 0;

		while (modifiers[i].name) {
		    if (modifMask & modifiers[i].mask) {
			modifMask &= ~modifiers[i].mask;
			modifierWidth += modifierCharWidth;
		    }
		    i++;
		}
		accelWidth = [[menuItem keyEquivalent] sizeWithAttributes:
			TkMacOSXNSFontAttributesForFont(tkfont)].width;
	    }
	    if (!mePtr->hideMargin) {
		indicatorSpace = menuMarkColumnWidth;
	    }
	    if (indicatorSpace > maxIndicatorSpace) {
		maxIndicatorSpace = indicatorSpace;
	    }
	    entryWidth = labelWidth + modifierWidth + accelWidth;
	    if (entryWidth > maxWidth) {
		maxWidth = entryWidth;
	    }
	    mePtr->height += 2 * activeBorderWidth;
	}
	mePtr->y = y;
	y += menuPtr->entries[i]->height + borderWidth;
	if (y > windowHeight) {
	    windowHeight = y;
	}
    }

    for (j = lastColumnBreak; j < menuPtr->numEntries; j++) {
	columnEntryPtr = menuPtr->entries[j];
	columnEntryPtr->indicatorSpace = maxIndicatorSpace;
	columnEntryPtr->width = maxIndicatorSpace + maxWidth
		+ 2 * activeBorderWidth;
	columnEntryPtr->x = x;
	columnEntryPtr->entryFlags |= ENTRY_LAST_COLUMN;
    }
    windowWidth = x + maxIndicatorSpace + maxWidth
	    + 2 * activeBorderWidth + borderWidth;
    windowHeight += borderWidth;

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
 * GenerateMenuSelectEvent --
 *
 *	Respond to a menu item being selected.
 *
 * Results:
 *	True if event(s) are generated - false otherwise.
 *
 * Side effects:
 *	Places a virtual event on the event queue.
 *
 *----------------------------------------------------------------------
 */

int
GenerateMenuSelectEvent(
    TKMenu *menu,
    NSMenuItem *menuItem)
{
    TkMenu *menuPtr = [menu tkMenu];

    if (menuPtr) {
	int index = [menu tkIndexOfItem:menuItem];

	if (index < 0 || index >= menuPtr->numEntries ||
		(menuPtr->entries[index])->state == ENTRY_DISABLED) {
	    TkActivateMenuEntry(menuPtr, -1);
	} else {
	    TkActivateMenuEntry(menuPtr, index);
	    MenuSelectEvent(menuPtr);
	    return true;
	}
    }
    return false;
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

void
MenuSelectEvent(
    TkMenu *menuPtr)		/* the menu we have selected. */
{
    XVirtualEvent event;

    bzero(&event, sizeof(XVirtualEvent));
    event.type = VirtualEvent;
    event.serial = LastKnownRequestProcessed(menuPtr->display);
    event.send_event = false;
    event.display = menuPtr->display;
    event.event = Tk_WindowId(menuPtr->tkwin);
    event.root = XRootWindow(menuPtr->display, 0);
    event.subwindow = None;
    event.time = TkpGetMS();
    XQueryPointer(NULL, None, NULL, NULL, &event.x_root, &event.y_root, NULL,
	    NULL, &event.state);
    event.same_screen = true;
    event.name = Tk_GetUid("MenuSelect");
    Tk_MakeWindowExist(menuPtr->tkwin);
    if (Tcl_GetServiceMode() != TCL_SERVICE_NONE) {
	Tk_HandleEvent((XEvent *) &event);
    } else {
	Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
    }
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

    TkActivateMenuEntry(menuPtr, -1);
    for (i = 0; i < menuPtr->numEntries; i++) {
	TkMenuEntry *mePtr = menuPtr->entries[i];

	if (mePtr->type == CASCADE_ENTRY
		&& (mePtr->childMenuRefPtr != NULL)
		&& (mePtr->childMenuRefPtr->menuPtr != NULL)) {
	    RecursivelyClearActiveMenu(mePtr->childMenuRefPtr->menuPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXClearMenubarActive --
 *
 *	Recursively clears the active entry in the current menubar hierarchy.
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
TkMacOSXClearMenubarActive(void)
{
    NSMenu *mainMenu = [NSApp mainMenu];

    if (mainMenu && [mainMenu isKindOfClass:[TKMenu class]]) {
	TkMenu *menuPtr = [(TKMenu *) mainMenu tkMenu];

	if (menuPtr && menuPtr->numEntries && menuPtr->entries) {
	    RecursivelyClearActiveMenu(menuPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MacOSXTurnOffMenus --
 *
 *	Turns off all the menu drawing code. This is more than just disabling
 *	the "menu" command, this means that Tk will NEVER touch the menubar.
 *	It is needed in the Plugin, where Tk does not own the menubar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A flag is set which will disable all menu drawing.
 *
 *----------------------------------------------------------------------
 */

void
Tk_MacOSXTurnOffMenus(void)
{
    gNoTkMenus = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuInit --
 *
 *	Initializes Mac-specific menu data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates a hash table.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuInit(void)
{
    TkColor *tkColPtr;

    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];

#define observe(n, s) \
	[nc addObserver:NSApp selector:@selector(s) name:(n) object:nil]
    observe(NSMenuDidBeginTrackingNotification, menuBeginTracking:);
    observe(NSMenuDidEndTrackingNotification, menuEndTracking:);
#undef observe

    [NSMenuItem setUsesUserKeyEquivalents:NO];
    tkColPtr = TkpGetColor(None, DEF_MENU_BG_COLOR);
    defaultBg = tkColPtr->color.pixel;
    ckfree(tkColPtr);
    tkColPtr = TkpGetColor(None, DEF_MENU_FG);
    defaultFg = tkColPtr->color.pixel;
    ckfree(tkColPtr);

    ChkErr(GetThemeMetric, kThemeMetricMenuMarkColumnWidth,
	    &menuMarkColumnWidth);
    ChkErr(GetThemeMetric, kThemeMetricMenuTextLeadingEdgeMargin,
	    &menuTextLeadingEdgeMargin);
    ChkErr(GetThemeMetric, kThemeMetricMenuTextTrailingEdgeMargin,
	    &menuTextTrailingEdgeMargin);
    ChkErr(GetThemeMetric, kThemeMetricMenuIconTrailingEdgeMargin,
	    &menuIconTrailingEdgeMargin);
    ChkErr(GetThemeMenuItemExtra, kThemeMenuItemPlain, &menuItemExtraHeight,
	    &menuItemExtraWidth);
    ChkErr(GetThemeMenuSeparatorHeight, &menuSeparatorHeight);
}

#pragma mark -
#pragma mark NOPs

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
 *----------------------------------------------------------------------
 *
 * TkpMenuNotifyToplevelCreate --
 *
 *	This routine reconfigures the menu and the clones indicated by
 *	menuName because a toplevel has been created and any system menus need
 *	to be created. Only applicable to Windows.
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
    TkpComputeStandardMenuGeometry(menuPtr);
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
}

#pragma mark Obsolete

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXPreprocessMenu --
 *
 *    Handle preprocessing of menubar if it exists.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    All post commands for the current menubar get executed.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXPreprocessMenu(void)
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXUseID --
 *
 *	Take the ID out of the available list for new menus. Used by the
 *	default menu bar's menus so that they do not get created at the tk
 *	level. See TkMacOSXGetNewMenuID for more information.
 *
 * Results:
 *	Returns TCL_OK if the id was not in use. Returns TCL_ERROR if the id
 *	was in use.
 *
 * Side effects:
 *	A hash table entry in the command table is created with a NULL value.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXUseMenuID(
    short macID)		/* The id to take out of the table */
{
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXDispatchMenuEvent --
 *
 *	Given a menu id and an item, dispatches the command associated with
 *	it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands for the event are scheduled for execution at idle time.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXDispatchMenuEvent(
    int menuID,			/* The menu id of the menu we are invoking */
    int index)			/* The one-based index of the item that was
				 * selected. */
{
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXHandleTearoffMenu() --
 *
 *	This routine sees if the MDEF has set a menu and a mouse position for
 *	tearing off and makes a tearoff menu if it has.
 *
 * Results:
 *	menuPtr->interp will have the result of the tearoff command.
 *
 * Side effects:
 *	A new tearoff menu is created if it is supposed to be.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXHandleTearoffMenu(void)
{
    /*
     * Obsolete: Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXSetHelpMenuItemCount --
 *
 *	Has to be called after the first call to InsertMenu. Sets up the
 *	global variable for the number of items in the unmodified help menu.
 *	NB. Nobody uses this any more, since you can get the number of system
 *	help items from HMGetHelpMenu trivially. But it is in the stubs
 *	table...
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Nothing.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXSetHelpMenuItemCount(void)
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXMenuClick --
 *
 *	Prepares a menubar for MenuSelect or MenuKey.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any pending configurations of the menubar are completed.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXMenuClick(void)
{
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
