/*
 * tkMacOSXPrivate.h --
 *
 *	Macros and declarations that are purely internal & private to TkAqua.
 *
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2008-2009, Apple Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 */

#ifndef _TKMACPRIV
#define _TKMACPRIV

#if !__OBJC__
#error Objective-C compiler required
#endif

#define TextStyle MacTextStyle
#import <ApplicationServices/ApplicationServices.h>
#import <Cocoa/Cocoa.h>
#ifndef NO_CARBON_H
#import <Carbon/Carbon.h>
#endif
#undef TextStyle
#import <objc/runtime.h> /* for sel_isEqual() */

#ifndef _TKMACINT
#include "tkMacOSXInt.h"
#endif
#ifndef _TKMACDEFAULT
#include "tkMacOSXDefault.h"
#endif

/* Macros for Mac OS X API availability checking */
#define TK_IF_MAC_OS_X_API(vers, symbol, ...) \
	tk_if_mac_os_x_10_##vers(symbol != NULL, 1, __VA_ARGS__)
#define TK_ELSE_MAC_OS_X(vers, ...) \
	tk_else_mac_os_x_10_##vers(__VA_ARGS__)
#define TK_IF_MAC_OS_X_API_COND(vers, symbol, cond, ...) \
	tk_if_mac_os_x_10_##vers(symbol != NULL, cond, __VA_ARGS__)
#define TK_ELSE(...) \
	} else { __VA_ARGS__
#define TK_ENDIF \
	}
/* Private macros that implement the checking macros above */
#define tk_if_mac_os_x_yes(chk, cond, ...) \
	if (cond) { __VA_ARGS__
#define tk_else_mac_os_x_yes(...) \
	} else {
#define tk_if_mac_os_x_chk(chk, cond, ...) \
	if ((chk) && (cond)) { __VA_ARGS__
#define tk_else_mac_os_x_chk(...) \
	} else { __VA_ARGS__
#define tk_if_mac_os_x_no(chk, cond, ...) \
	if (0) {
#define tk_else_mac_os_x_no(...) \
	} else { __VA_ARGS__

/*
 * Macros for DEBUG_ASSERT_MESSAGE et al from Debugging.h.
 */

#undef kComponentSignatureString
#undef COMPONENT_SIGNATURE
#define kComponentSignatureString "TkMacOSX"
#define COMPONENT_SIGNATURE 'Tk  '

/*
 * Macros abstracting checks only active in a debug build.
 */

#ifdef TK_MAC_DEBUG
#define TKLog(f, ...) NSLog(f, ##__VA_ARGS__)

/*
 * Macro to do debug message output.
 */
#define TkMacOSXDbgMsg(m, ...) \
    do { \
	TKLog(@"%s:%d: %s(): " m, strrchr(__FILE__, '/')+1, \
		__LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

/*
 * Macro to do debug API failure message output.
 */
#define TkMacOSXDbgOSErr(f, err) \
    do { \
	TkMacOSXDbgMsg("%s failed: %d", #f, (int)(err)); \
    } while (0)

/*
 * Macro to do very common check for noErr return from given API and output
 * debug message in case of failure.
 */
#define ChkErr(f, ...) ({ \
	OSStatus err = f(__VA_ARGS__); \
	if (err != noErr) { \
	    TkMacOSXDbgOSErr(f, err); \
	} \
	err;})

#else /* TK_MAC_DEBUG */
#define TKLog(f, ...)
#define TkMacOSXDbgMsg(m, ...)
#define TkMacOSXDbgOSErr(f, err)
#define ChkErr(f, ...) ({f(__VA_ARGS__);})
#endif /* TK_MAC_DEBUG */

/*
 * Macro abstracting use of TkMacOSXGetNamedSymbol to init named symbols.
 */

#define TkMacOSXInitNamedSymbol(module, ret, symbol, ...) \
    static ret (* symbol)(__VA_ARGS__) = (void*)(-1L); \
    if (symbol == (void*)(-1L)) { \
	symbol = TkMacOSXGetNamedSymbol(STRINGIFY(module), \
		STRINGIFY(symbol)); \
    }

/*
 * Structure encapsulating current drawing environment.
 */

typedef struct TkMacOSXDrawingContext {
    CGContextRef context;
    NSView *view;
    HIShapeRef clipRgn;
    CGRect portBounds;
} TkMacOSXDrawingContext;

/*
 * Variables internal to TkAqua.
 */

MODULE_SCOPE long tkMacOSXMacOSXVersion;

/*
 * Prototypes for TkMacOSXRegion.c.
 */

#if 0
MODULE_SCOPE void	TkMacOSXEmtpyRegion(TkRegion r);
MODULE_SCOPE int	TkMacOSXIsEmptyRegion(TkRegion r);
#endif
MODULE_SCOPE HIShapeRef	TkMacOSXGetNativeRegion(TkRegion r);
MODULE_SCOPE void	TkMacOSXSetWithNativeRegion(TkRegion r,
			    HIShapeRef rgn);
MODULE_SCOPE void	TkMacOSXOffsetRegion(TkRegion r, short dx, short dy);
MODULE_SCOPE HIShapeRef	TkMacOSXHIShapeCreateEmpty(void);
MODULE_SCOPE HIMutableShapeRef TkMacOSXHIShapeCreateMutableWithRect(
			    const CGRect *inRect);
MODULE_SCOPE OSStatus	TkMacOSXHIShapeSetWithShape(
			    HIMutableShapeRef inDestShape,
			    HIShapeRef inSrcShape);
#if 0
MODULE_SCOPE OSStatus	TkMacOSXHIShapeSetWithRect(HIMutableShapeRef inShape,
			    const CGRect *inRect);
#endif
MODULE_SCOPE OSStatus	TkMacOSHIShapeDifferenceWithRect(
			    HIMutableShapeRef inShape, const CGRect *inRect);
MODULE_SCOPE OSStatus	TkMacOSHIShapeUnionWithRect(HIMutableShapeRef inShape,
			    const CGRect *inRect);
MODULE_SCOPE OSStatus	TkMacOSHIShapeUnion(HIShapeRef inShape1,
			    HIShapeRef inShape2, HIMutableShapeRef outResult);

/*
 * Prototypes of TkAqua internal procs.
 */

MODULE_SCOPE void *	TkMacOSXGetNamedSymbol(const char *module,
			    const char *symbol);
MODULE_SCOPE void	TkMacOSXDisplayChanged(Display *display);
MODULE_SCOPE CGFloat	TkMacOSXZeroScreenHeight();
MODULE_SCOPE CGFloat	TkMacOSXZeroScreenTop();
MODULE_SCOPE int	TkMacOSXUseAntialiasedText(Tcl_Interp *interp,
			    int enable);
MODULE_SCOPE int	TkMacOSXInitCGDrawing(Tcl_Interp *interp, int enable,
			    int antiAlias);
MODULE_SCOPE int	TkMacOSXGenerateFocusEvent(TkWindow *winPtr,
			    int activeFlag);
MODULE_SCOPE WindowClass TkMacOSXWindowClass(TkWindow *winPtr);
MODULE_SCOPE int	TkMacOSXIsWindowZoomed(TkWindow *winPtr);
MODULE_SCOPE int	TkGenerateButtonEventForXPointer(Window window);
MODULE_SCOPE EventModifiers TkMacOSXModifierState(void);
MODULE_SCOPE NSBitmapImageRep* TkMacOSXBitmapRepFromDrawableRect(Drawable drawable,
			    int x, int y, unsigned int width, unsigned int height);
MODULE_SCOPE CGImageRef TkMacOSXCreateCGImageWithXImage(XImage *image);
MODULE_SCOPE void       TkMacOSXDrawCGImage(Drawable d, GC gc, CGContextRef context,
			    CGImageRef image, unsigned long imageForeground,
			    unsigned long imageBackground, CGRect imageBounds,
			    CGRect srcBounds, CGRect dstBounds);
MODULE_SCOPE int	TkMacOSXSetupDrawingContext(Drawable d, GC gc,
			    int useCG, TkMacOSXDrawingContext *dcPtr);
MODULE_SCOPE void	TkMacOSXRestoreDrawingContext(
			    TkMacOSXDrawingContext *dcPtr);
MODULE_SCOPE void	TkMacOSXSetColorInContext(GC gc, unsigned long pixel,
			    CGContextRef context);
MODULE_SCOPE int	TkMacOSXMakeFullscreen(TkWindow *winPtr,
			    NSWindow *window, int fullscreen,
			    Tcl_Interp *interp);
MODULE_SCOPE void	TkMacOSXEnterExitFullscreen(TkWindow *winPtr,
			    int active);
MODULE_SCOPE NSWindow*	TkMacOSXDrawableWindow(Drawable drawable);
MODULE_SCOPE NSView*	TkMacOSXDrawableView(MacDrawable *macWin);
MODULE_SCOPE void	TkMacOSXWinCGBounds(TkWindow *winPtr, CGRect *bounds);
MODULE_SCOPE HIShapeRef	TkMacOSXGetClipRgn(Drawable drawable);
MODULE_SCOPE void	TkMacOSXInvalidateViewRegion(NSView *view,
			    HIShapeRef rgn);
MODULE_SCOPE CGContextRef TkMacOSXGetCGContextForDrawable(Drawable drawable);
MODULE_SCOPE CGImageRef	TkMacOSXCreateCGImageWithDrawable(Drawable drawable);
MODULE_SCOPE NSImage*	TkMacOSXGetNSImageWithTkImage(Display *display,
			    Tk_Image image, int width, int height);
MODULE_SCOPE NSImage*	TkMacOSXGetNSImageWithBitmap(Display *display,
			    Pixmap bitmap, GC gc, int width, int height);
MODULE_SCOPE CGColorRef	TkMacOSXCreateCGColor(GC gc, unsigned long pixel);
MODULE_SCOPE NSColor*	TkMacOSXGetNSColor(GC gc, unsigned long pixel);
MODULE_SCOPE Tcl_Obj *	TkMacOSXGetStringObjFromCFString(CFStringRef str);
MODULE_SCOPE TkWindow*	TkMacOSXGetTkWindow(NSWindow *w);
MODULE_SCOPE NSFont*	TkMacOSXNSFontForFont(Tk_Font tkfont);
MODULE_SCOPE NSDictionary* TkMacOSXNSFontAttributesForFont(Tk_Font tkfont);
MODULE_SCOPE NSModalSession TkMacOSXGetModalSession(void);
MODULE_SCOPE void	TkMacOSXSelDeadWindow(TkWindow *winPtr);
MODULE_SCOPE void	TkMacOSXApplyWindowAttributes(TkWindow *winPtr,
			    NSWindow *macWindow);
MODULE_SCOPE int	TkMacOSXStandardAboutPanelObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
MODULE_SCOPE int	TkMacOSXIconBitmapObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
MODULE_SCOPE void       TkMacOSXDrawSolidBorder(Tk_Window tkwin, GC gc,
			    int inset, int thickness);
MODULE_SCOPE int 	TkMacOSXServices_Init(Tcl_Interp *interp);
MODULE_SCOPE int	TkMacOSXRegisterServiceWidgetObjCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
MODULE_SCOPE NSString*  TclUniToNSString(const char *source, int numBytes);
MODULE_SCOPE int        TclUniAtIndex(NSString *string, int index, char *uni,
				      unsigned int *code);
MODULE_SCOPE char*      NSStringToTclUni(NSString *string, int *numBytes);

#pragma mark Private Objective-C Classes

#define VISIBILITY_HIDDEN __attribute__((visibility("hidden")))

enum { tkMainMenu = 1, tkApplicationMenu, tkWindowsMenu, tkHelpMenu};

VISIBILITY_HIDDEN
@interface TKMenu : NSMenu {
@private
    void *_tkMenu;
    NSUInteger _tkOffset, _tkItemCount, _tkSpecial;
}
- (void)setSpecial:(NSUInteger)special;
- (BOOL)isSpecial:(NSUInteger)special;
@end

@interface TKMenu(TKMenuDelegate) <NSMenuDelegate>
@end

VISIBILITY_HIDDEN
@interface TKApplication : NSApplication {
@private
    Tcl_Interp *_eventInterp;
    NSMenu *_servicesMenu;
    TKMenu *_defaultMainMenu, *_defaultApplicationMenu;
    NSMenuItem *_demoMenuItem;
    NSArray *_defaultApplicationMenuItems, *_defaultWindowsMenuItems;
    NSArray *_defaultHelpMenuItems, *_defaultFileMenuItems;
    NSAutoreleasePool *_mainPool;
#ifdef __i386__
    /* The Objective C runtime used on i386 requires this. */
    int _poolLock;
    int _macMinorVersion;
    Bool _isDrawing;
#endif
}
@property int poolLock;
@property int macMinorVersion;
@property Bool isDrawing;

@end
@interface TKApplication(TKInit)
- (NSString *)tkFrameworkImagePath:(NSString*)image;
- (void)_resetAutoreleasePool;
- (void)_lockAutoreleasePool;
- (void)_unlockAutoreleasePool;
@end
@interface TKApplication(TKKeyboard)
- (void) keyboardChanged: (NSNotification *) notification;
@end
@interface TKApplication(TKWindowEvent) <NSApplicationDelegate>
- (void) _setupWindowNotifications;
@end
@interface TKApplication(TKDialog) <NSOpenSavePanelDelegate>
@end
@interface TKApplication(TKMenu)
- (void)tkSetMainMenu:(TKMenu *)menu;
@end
@interface TKApplication(TKMenus)
- (void) _setupMenus;
@end
@interface NSApplication(TKNotify)
/* We need to declare this hidden method. */
- (void) _modalSession: (NSModalSession) session sendEvent: (NSEvent *) event;
@end
@interface TKApplication(TKEvent)
- (NSEvent *)tkProcessEvent:(NSEvent *)theEvent;
@end
@interface TKApplication(TKMouseEvent)
- (NSEvent *)tkProcessMouseEvent:(NSEvent *)theEvent;
@end
@interface TKApplication(TKKeyEvent)
- (NSEvent *)tkProcessKeyEvent:(NSEvent *)theEvent;
@end
@interface TKApplication(TKClipboard)
- (void)tkProvidePasteboard:(TkDisplay *)dispPtr;
- (void)tkCheckPasteboard;
@end
@interface TKApplication(TKHLEvents)
- (void) terminate: (id) sender;
- (void) preferences: (id) sender;
- (void) handleQuitApplicationEvent:   (NSAppleEventDescriptor *)event
		     withReplyEvent:   (NSAppleEventDescriptor *)replyEvent;
- (void) handleOpenApplicationEvent:   (NSAppleEventDescriptor *)event
		     withReplyEvent:   (NSAppleEventDescriptor *)replyEvent;
- (void) handleReopenApplicationEvent: (NSAppleEventDescriptor *)event
		       withReplyEvent: (NSAppleEventDescriptor *)replyEvent;
- (void) handleShowPreferencesEvent:   (NSAppleEventDescriptor *)event
		     withReplyEvent:   (NSAppleEventDescriptor *)replyEvent;
- (void) handleOpenDocumentsEvent:     (NSAppleEventDescriptor *)event
		   withReplyEvent:     (NSAppleEventDescriptor *)replyEvent;
- (void) handlePrintDocumentsEvent:    (NSAppleEventDescriptor *)event
		   withReplyEvent:     (NSAppleEventDescriptor *)replyEvent;
- (void) handleDoScriptEvent:          (NSAppleEventDescriptor *)event
		   withReplyEvent:     (NSAppleEventDescriptor *)replyEvent;
- (void)handleURLEvent:                (NSAppleEventDescriptor*)event
	           withReplyEvent:     (NSAppleEventDescriptor*)replyEvent;
@end

VISIBILITY_HIDDEN
/*
 * Subclass TKContentView from NSTextInputClient to enable composition and
 * input from the Character Palette.
 */

@interface TKContentView : NSView <NSTextInputClient>
{
@private
    NSString *privateWorkingText;
    Bool _needsRedisplay;
}
@property Bool needsRedisplay;
@end

@interface TKContentView(TKKeyEvent)
- (void) deleteWorkingText;
@end

@interface TKContentView(TKWindowEvent)
- (void) generateExposeEvents: (HIShapeRef) shape;
- (void) tkToolbarButton: (id) sender;
@end

@interface NSWindow(TKWm)
- (NSPoint) tkConvertPointToScreen:(NSPoint)point;
- (NSPoint) tkConvertPointFromScreen:(NSPoint)point;
@end

VISIBILITY_HIDDEN
@interface TKWindow : NSWindow
@end

@interface TKWindow(TKWm)
- (void)    tkLayoutChanged;
@end

@interface NSDrawerWindow : NSWindow
{
    id _i1, _i2;
}
@end

#pragma mark NSMenu & NSMenuItem Utilities

@interface NSMenu(TKUtils)
+ (id)menuWithTitle:(NSString *)title;
+ (id)menuWithTitle:(NSString *)title menuItems:(NSArray *)items;
+ (id)menuWithTitle:(NSString *)title submenus:(NSArray *)submenus;
- (NSMenuItem *)itemWithSubmenu:(NSMenu *)submenu;
- (NSMenuItem *)itemInSupermenu;
@end

@interface NSMenuItem(TKUtils)
+ (id)itemWithSubmenu:(NSMenu *)submenu;
+ (id)itemWithTitle:(NSString *)title submenu:(NSMenu *)submenu;
+ (id)itemWithTitle:(NSString *)title action:(SEL)action;
+ (id)itemWithTitle:(NSString *)title action:(SEL)action
	target:(id)target;
+ (id)itemWithTitle:(NSString *)title action:(SEL)action
	keyEquivalent:(NSString *)keyEquivalent;
+ (id)itemWithTitle:(NSString *)title action:(SEL)action
	target:(id)target keyEquivalent:(NSString *)keyEquivalent;
+ (id)itemWithTitle:(NSString *)title action:(SEL)action
	keyEquivalent:(NSString *)keyEquivalent
	keyEquivalentModifierMask:(NSUInteger)keyEquivalentModifierMask;
+ (id)itemWithTitle:(NSString *)title action:(SEL)action
	target:(id)target keyEquivalent:(NSString *)keyEquivalent
	keyEquivalentModifierMask:(NSUInteger)keyEquivalentModifierMask;
@end

@interface NSColorPanel(TKDialog)
- (void) _setUseModalAppearance: (BOOL) flag;
@end

@interface NSFont(TKFont)
- (NSFont *) bestMatchingFontForCharacters: (const UTF16Char *) characters
	length: (NSUInteger) length attributes: (NSDictionary *) attributes
	actualCoveredLength: (NSUInteger *) coveredLength;
@end

/*
 * This method of NSApplication is not declared in NSApplication.h so we
 * declare it here to be a method of the TKMenu category.
 */

@interface NSApplication(TKMenu)
- (void) setAppleMenu: (NSMenu *) menu;
@end

#endif /* _TKMACPRIV */

int TkMacOSXGetAppPath(ClientData cd, Tcl_Interp *ip, int objc, Tcl_Obj *const objv[]);

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
