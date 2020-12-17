/*
 * tkStubInit.c --
 *
 *	This file contains the initializers for the Tk stub vectors.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

#if !(defined(_WIN32) || defined(MAC_OSX_TK))
/* UNIX */
#define UNIX_TK
#include "tkUnixInt.h"
#endif

#ifdef _WIN32
#include "tkWinInt.h"
#endif

#if defined(MAC_OSX_TK)
/* we could have used _TKMACINT */
#include "tkMacOSXInt.h"
#endif

/* TODO: These ought to come in some other way */
#include "tkPlatDecls.h"
#include "tkIntXlibDecls.h"

static const TkIntStubs tkIntStubs;
MODULE_SCOPE const TkStubs tkStubs;

/*
 * Remove macro that might interfere with the definition below.
 */

#undef Tk_MainEx
#undef XVisualIDFromVisual
#undef XSynchronize
#undef XUngrabServer
#undef XNoOp
#undef XGrabServer
#undef XFree
#undef XFlush

#ifdef _WIN32

int
TkpCmapStressed(Tk_Window tkwin, Colormap colormap)
{
    /* dummy implementation, no need to do anything */
    return 0;
}
void
TkpSync(Display *display)
{
    /* dummy implementation, no need to do anything */
}

void
TkCreateXEventSource(void)
{
    TkWinXInit(Tk_GetHINSTANCE());
}

#   define TkUnixContainerId 0
#   define TkUnixDoOneXEvent 0
#   define TkUnixSetMenubar 0
#   define XCreateWindow 0
#   define XOffsetRegion 0
#   define XUnionRegion 0
#   define TkWmCleanup (void (*)(TkDisplay *)) TkpSync
#   define TkSendCleanup (void (*)(TkDisplay *)) TkpSync
#   define TkpTestsendCmd 0

#else /* !_WIN32 */

/*
 * Make sure that extensions which call XParseColor through the stub
 * table, call TkParseColor instead. [Bug 3486474]
 */
#   define XParseColor	TkParseColor

#   ifdef __CYGWIN__

/*
 * Trick, so we don't have to include <windows.h> here, which in any
 * case lacks this function anyway.
 */

#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS	0x00000004
int __stdcall GetModuleHandleExW(unsigned int, const char *, void *);

void *Tk_GetHINSTANCE()
{
    void *hInstance = NULL;

    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
	    (const char *) &tkIntStubs, &hInstance);
    return hInstance;
}

void
TkSetPixmapColormap(
    Pixmap pixmap,
    Colormap colormap)
{
}

void
TkpPrintWindowId(
    char *buf,			/* Pointer to string large enough to hold
				 * the hex representation of a pointer. */
    Window window)		/* Window to be printed into buffer. */
{
    sprintf(buf, "0x%" TCL_Z_MODIFIER "x", (size_t)window);
}

int
TkPutImage(
    unsigned long *colors,	/* Array of pixel values used by this image.
				 * May be NULL. */
    int ncolors,		/* Number of colors used, or 0. */
    Display *display,
    Drawable d,			/* Destination drawable. */
    GC gc,
    XImage *image,		/* Source image. */
    int src_x, int src_y,	/* Offset of subimage. */
    int dest_x, int dest_y,	/* Position of subimage origin in drawable. */
    unsigned int width, unsigned int height)
				/* Dimensions of subimage. */
{
    return XPutImage(display, d, gc, image, src_x, src_y, dest_x, dest_y, width, height);
}

TkRegion TkCreateRegion()
{
    return (TkRegion) XCreateRegion();
}

int TkDestroyRegion(TkRegion r)
{
	return XDestroyRegion((Region)r);
}

int TkSetRegion(Display *d, GC g, TkRegion r)
{
	return XSetRegion(d, g, (Region)r);
}

int TkUnionRectWithRegion(XRectangle *a, TkRegion b, TkRegion c)
{
	return XUnionRectWithRegion(a, (Region) b, (Region) c);
}

int TkClipBox(TkRegion a, XRectangle *b)
{
	return XClipBox((Region) a, b);
}

int TkIntersectRegion(TkRegion a, TkRegion b, TkRegion c)
{
	return XIntersectRegion((Region) a, (Region) b, (Region) c);
}

int TkRectInRegion (TkRegion r, int a, int b, unsigned int c, unsigned int d)
{
    return XRectInRegion((Region) r, a, b, c, d);
}

int TkSubtractRegion (TkRegion a, TkRegion b, TkRegion c)
{
    return XSubtractRegion((Region) a, (Region) b, (Region) c);
}

	/* TODO: To be implemented for Cygwin */
#	define Tk_AttachHWND 0
#	define Tk_GetHWND 0
#	define Tk_HWNDToWindow 0
#	define Tk_PointerEvent 0
#	define Tk_TranslateWinEvent 0
#	define TkAlignImageData 0
#	define TkGenerateActivateEvents 0
#	define TkpGetMS 0
#	define TkpGetCapture 0
#	define TkPointerDeadWindow 0
#	define TkpSetCapture 0
#	define TkpSetCursor 0
#	define TkWinCancelMouseTimer 0
#	define TkWinClipboardRender 0
#	define TkWinEmbeddedEventProc 0
#	define TkWinFillRect 0
#	define TkWinGetBorderPixels 0
#	define TkWinGetDrawableDC 0
#	define TkWinGetModifierState 0
#	define TkWinGetSystemPalette 0
#	define TkWinGetWrapperWindow 0
#	define TkWinHandleMenuEvent 0
#	define TkWinIndexOfColor 0
#	define TkWinReleaseDrawableDC 0
#	define TkWinResendEvent 0
#	define TkWinSelectPalette 0
#	define TkWinSetMenu 0
#	define TkWinSetWindowPos 0
#	define TkWinWmCleanup 0
#	define TkWinXCleanup 0
#	define TkWinXInit 0
#	define TkWinSetForegroundWindow 0
#	define TkWinDialogDebug 0
#	define TkWinGetMenuSystemDefault 0
#	define TkWinGetPlatformId 0
#	define TkWinSetHINSTANCE 0
#	define TkWinGetPlatformTheme 0
#	define TkWinChildProc 0

#   elif !defined(MAC_OSX_TK) /* UNIX */

#	undef TkClipBox
#	undef TkCreateRegion
#	undef TkDestroyRegion
#	undef TkIntersectRegion
#	undef TkRectInRegion
#	undef TkSetRegion
#	undef TkUnionRectWithRegion
#	undef TkSubtractRegion

#	define TkClipBox (int (*) (TkRegion, XRectangle *)) XClipBox
#	define TkCreateRegion (TkRegion (*) ()) XCreateRegion
#	define TkDestroyRegion (int (*) (TkRegion)) XDestroyRegion
#	define TkIntersectRegion (int (*) (TkRegion, TkRegion, TkRegion)) XIntersectRegion
#	define TkRectInRegion (int (*) (TkRegion, int, int, unsigned int, unsigned int)) XRectInRegion
#	define TkSetRegion (int (*) (Display *, GC, TkRegion)) XSetRegion
#	define TkUnionRectWithRegion (int (*) (XRectangle *, TkRegion, TkRegion)) XUnionRectWithRegion
#	define TkSubtractRegion (int (*) (TkRegion, TkRegion, TkRegion)) XSubtractRegion
#   endif
#endif /* !_WIN32 */

/*
 * WARNING: The contents of this file is automatically generated by the
 * tools/genStubs.tcl script. Any modifications to the function declarations
 * below should be made in the generic/tk.decls script.
 */

/* !BEGIN!: Do not edit below this line. */

static const TkIntStubs tkIntStubs = {
    TCL_STUB_MAGIC,
    0,
    TkAllocWindow, /* 0 */
    TkBezierPoints, /* 1 */
    TkBezierScreenPoints, /* 2 */
    0, /* 3 */
    TkBindEventProc, /* 4 */
    TkBindFree, /* 5 */
    TkBindInit, /* 6 */
    TkChangeEventWindow, /* 7 */
    TkClipInit, /* 8 */
    TkComputeAnchor, /* 9 */
    0, /* 10 */
    0, /* 11 */
    TkCreateCursorFromData, /* 12 */
    TkCreateFrame, /* 13 */
    TkCreateMainWindow, /* 14 */
    TkCurrentTime, /* 15 */
    TkDeleteAllImages, /* 16 */
    TkDoConfigureNotify, /* 17 */
    TkDrawInsetFocusHighlight, /* 18 */
    TkEventDeadWindow, /* 19 */
    TkFillPolygon, /* 20 */
    TkFindStateNum, /* 21 */
    TkFindStateString, /* 22 */
    TkFocusDeadWindow, /* 23 */
    TkFocusFilterEvent, /* 24 */
    TkFocusKeyEvent, /* 25 */
    TkFontPkgInit, /* 26 */
    TkFontPkgFree, /* 27 */
    TkFreeBindingTags, /* 28 */
    TkpFreeCursor, /* 29 */
    TkGetBitmapData, /* 30 */
    TkGetButtPoints, /* 31 */
    TkGetCursorByName, /* 32 */
    TkGetDefaultScreenName, /* 33 */
    TkGetDisplay, /* 34 */
    TkGetDisplayOf, /* 35 */
    TkGetFocusWin, /* 36 */
    TkGetInterpNames, /* 37 */
    TkGetMiterPoints, /* 38 */
    TkGetPointerCoords, /* 39 */
    TkGetServerInfo, /* 40 */
    TkGrabDeadWindow, /* 41 */
    TkGrabState, /* 42 */
    TkIncludePoint, /* 43 */
    TkInOutEvents, /* 44 */
    TkInstallFrameMenu, /* 45 */
    TkKeysymToString, /* 46 */
    TkLineToArea, /* 47 */
    TkLineToPoint, /* 48 */
    TkMakeBezierCurve, /* 49 */
    TkMakeBezierPostscript, /* 50 */
    TkOptionClassChanged, /* 51 */
    TkOptionDeadWindow, /* 52 */
    TkOvalToArea, /* 53 */
    TkOvalToPoint, /* 54 */
    TkpChangeFocus, /* 55 */
    TkpCloseDisplay, /* 56 */
    TkpClaimFocus, /* 57 */
    TkpDisplayWarning, /* 58 */
    TkpGetAppName, /* 59 */
    TkpGetOtherWindow, /* 60 */
    TkpGetWrapperWindow, /* 61 */
    TkpInit, /* 62 */
    TkpInitializeMenuBindings, /* 63 */
    TkpMakeContainer, /* 64 */
    TkpMakeMenuWindow, /* 65 */
    TkpMakeWindow, /* 66 */
    TkpMenuNotifyToplevelCreate, /* 67 */
    TkpOpenDisplay, /* 68 */
    TkPointerEvent, /* 69 */
    TkPolygonToArea, /* 70 */
    TkPolygonToPoint, /* 71 */
    TkPositionInTree, /* 72 */
    TkpRedirectKeyEvent, /* 73 */
    TkpSetMainMenubar, /* 74 */
    TkpUseWindow, /* 75 */
    0, /* 76 */
    TkQueueEventForAllChildren, /* 77 */
    TkReadBitmapFile, /* 78 */
    TkScrollWindow, /* 79 */
    TkSelDeadWindow, /* 80 */
    TkSelEventProc, /* 81 */
    TkSelInit, /* 82 */
    TkSelPropProc, /* 83 */
    0, /* 84 */
    TkSetWindowMenuBar, /* 85 */
    TkStringToKeysym, /* 86 */
    TkThickPolyLineToArea, /* 87 */
    TkWmAddToColormapWindows, /* 88 */
    TkWmDeadWindow, /* 89 */
    TkWmFocusToplevel, /* 90 */
    TkWmMapWindow, /* 91 */
    TkWmNewWindow, /* 92 */
    TkWmProtocolEventProc, /* 93 */
    TkWmRemoveFromColormapWindows, /* 94 */
    TkWmRestackToplevel, /* 95 */
    TkWmSetClass, /* 96 */
    TkWmUnmapWindow, /* 97 */
    TkDebugBitmap, /* 98 */
    TkDebugBorder, /* 99 */
    TkDebugCursor, /* 100 */
    TkDebugColor, /* 101 */
    TkDebugConfig, /* 102 */
    TkDebugFont, /* 103 */
    TkFindStateNumObj, /* 104 */
    TkGetBitmapPredefTable, /* 105 */
    TkGetDisplayList, /* 106 */
    TkGetMainInfoList, /* 107 */
    TkGetWindowFromObj, /* 108 */
    TkpGetString, /* 109 */
    TkpGetSubFonts, /* 110 */
    TkpGetSystemDefault, /* 111 */
    TkpMenuThreadInit, /* 112 */
    TkClipBox, /* 113 */
    TkCreateRegion, /* 114 */
    TkDestroyRegion, /* 115 */
    TkIntersectRegion, /* 116 */
    TkRectInRegion, /* 117 */
    TkSetRegion, /* 118 */
    TkUnionRectWithRegion, /* 119 */
    0, /* 120 */
#if !(defined(_WIN32) || defined(MAC_OSX_TK)) /* X11 */
    0, /* 121 */
#endif /* X11 */
#if defined(_WIN32) /* WIN */
    0, /* 121 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    0, /* 121 */ /* Dummy entry for stubs table backwards compatibility */
    TkpCreateNativeBitmap, /* 121 */
#endif /* AQUA */
#if !(defined(_WIN32) || defined(MAC_OSX_TK)) /* X11 */
    0, /* 122 */
#endif /* X11 */
#if defined(_WIN32) /* WIN */
    0, /* 122 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    0, /* 122 */ /* Dummy entry for stubs table backwards compatibility */
    TkpDefineNativeBitmaps, /* 122 */
#endif /* AQUA */
    0, /* 123 */
#if !(defined(_WIN32) || defined(MAC_OSX_TK)) /* X11 */
    0, /* 124 */
#endif /* X11 */
#if defined(_WIN32) /* WIN */
    0, /* 124 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    0, /* 124 */ /* Dummy entry for stubs table backwards compatibility */
    TkpGetNativeAppBitmap, /* 124 */
#endif /* AQUA */
    0, /* 125 */
    0, /* 126 */
    0, /* 127 */
    0, /* 128 */
    0, /* 129 */
    0, /* 130 */
    0, /* 131 */
    0, /* 132 */
    0, /* 133 */
    0, /* 134 */
    TkpDrawHighlightBorder, /* 135 */
    TkSetFocusWin, /* 136 */
    TkpSetKeycodeAndState, /* 137 */
    TkpGetKeySym, /* 138 */
    TkpInitKeymapInfo, /* 139 */
    TkPhotoGetValidRegion, /* 140 */
    TkWmStackorderToplevel, /* 141 */
    TkFocusFree, /* 142 */
    TkClipCleanup, /* 143 */
    TkGCCleanup, /* 144 */
    TkSubtractRegion, /* 145 */
    TkStylePkgInit, /* 146 */
    TkStylePkgFree, /* 147 */
    TkToplevelWindowForCommand, /* 148 */
    TkGetOptionSpec, /* 149 */
    TkMakeRawCurve, /* 150 */
    TkMakeRawCurvePostscript, /* 151 */
    TkpDrawFrame, /* 152 */
    TkCreateThreadExitHandler, /* 153 */
    TkDeleteThreadExitHandler, /* 154 */
    0, /* 155 */
    TkpTestembedCmd, /* 156 */
    TkpTesttextCmd, /* 157 */
    TkSelGetSelection, /* 158 */
    TkTextGetIndex, /* 159 */
    TkTextIndexBackBytes, /* 160 */
    TkTextIndexForwBytes, /* 161 */
    TkTextMakeByteIndex, /* 162 */
    TkTextPrintIndex, /* 163 */
    TkTextSetMark, /* 164 */
    TkTextXviewCmd, /* 165 */
    TkTextChanged, /* 166 */
    TkBTreeNumLines, /* 167 */
    TkTextInsertDisplayProc, /* 168 */
    TkStateParseProc, /* 169 */
    TkStatePrintProc, /* 170 */
    TkCanvasDashParseProc, /* 171 */
    TkCanvasDashPrintProc, /* 172 */
    TkOffsetParseProc, /* 173 */
    TkOffsetPrintProc, /* 174 */
    TkPixelParseProc, /* 175 */
    TkPixelPrintProc, /* 176 */
    TkOrientParseProc, /* 177 */
    TkOrientPrintProc, /* 178 */
    TkSmoothParseProc, /* 179 */
    TkSmoothPrintProc, /* 180 */
    TkDrawAngledTextLayout, /* 181 */
    TkUnderlineAngledTextLayout, /* 182 */
    TkIntersectAngledTextLayout, /* 183 */
    TkDrawAngledChars, /* 184 */
};

static const TkIntPlatStubs tkIntPlatStubs = {
    TCL_STUB_MAGIC,
    0,
#if defined(_WIN32) || defined(__CYGWIN__) /* WIN */
    TkAlignImageData, /* 0 */
    0, /* 1 */
    TkGenerateActivateEvents, /* 2 */
    TkpGetMS, /* 3 */
    TkPointerDeadWindow, /* 4 */
    TkpPrintWindowId, /* 5 */
    TkpScanWindowId, /* 6 */
    TkpSetCapture, /* 7 */
    TkpSetCursor, /* 8 */
    TkpWmSetState, /* 9 */
    TkSetPixmapColormap, /* 10 */
    TkWinCancelMouseTimer, /* 11 */
    TkWinClipboardRender, /* 12 */
    TkWinEmbeddedEventProc, /* 13 */
    TkWinFillRect, /* 14 */
    TkWinGetBorderPixels, /* 15 */
    TkWinGetDrawableDC, /* 16 */
    TkWinGetModifierState, /* 17 */
    TkWinGetSystemPalette, /* 18 */
    TkWinGetWrapperWindow, /* 19 */
    TkWinHandleMenuEvent, /* 20 */
    TkWinIndexOfColor, /* 21 */
    TkWinReleaseDrawableDC, /* 22 */
    TkWinResendEvent, /* 23 */
    TkWinSelectPalette, /* 24 */
    TkWinSetMenu, /* 25 */
    TkWinSetWindowPos, /* 26 */
    TkWinWmCleanup, /* 27 */
    TkWinXCleanup, /* 28 */
    TkWinXInit, /* 29 */
    TkWinSetForegroundWindow, /* 30 */
    TkWinDialogDebug, /* 31 */
    TkWinGetMenuSystemDefault, /* 32 */
    TkWinGetPlatformId, /* 33 */
    TkWinSetHINSTANCE, /* 34 */
    TkWinGetPlatformTheme, /* 35 */
    TkWinChildProc, /* 36 */
    TkCreateXEventSource, /* 37 */
    TkpCmapStressed, /* 38 */
    TkpSync, /* 39 */
    TkUnixContainerId, /* 40 */
    TkUnixDoOneXEvent, /* 41 */
    TkUnixSetMenubar, /* 42 */
    TkWmCleanup, /* 43 */
    TkSendCleanup, /* 44 */
    TkpTestsendCmd, /* 45 */
    0, /* 46 */
    TkpGetCapture, /* 47 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    TkGenerateActivateEvents, /* 0 */
    0, /* 1 */
    0, /* 2 */
    TkPointerDeadWindow, /* 3 */
    TkpSetCapture, /* 4 */
    TkpSetCursor, /* 5 */
    TkpWmSetState, /* 6 */
    TkAboutDlg, /* 7 */
    TkMacOSXButtonKeyState, /* 8 */
    TkMacOSXClearMenubarActive, /* 9 */
    TkMacOSXDispatchMenuEvent, /* 10 */
    TkMacOSXInstallCursor, /* 11 */
    TkMacOSXHandleTearoffMenu, /* 12 */
    0, /* 13 */
    TkMacOSXDoHLEvent, /* 14 */
    0, /* 15 */
    TkMacOSXGetXWindow, /* 16 */
    TkMacOSXGrowToplevel, /* 17 */
    TkMacOSXHandleMenuSelect, /* 18 */
    0, /* 19 */
    0, /* 20 */
    TkMacOSXInvalidateWindow, /* 21 */
    TkMacOSXIsCharacterMissing, /* 22 */
    TkMacOSXMakeRealWindowExist, /* 23 */
    TkMacOSXMakeStippleMap, /* 24 */
    TkMacOSXMenuClick, /* 25 */
    TkMacOSXRegisterOffScreenWindow, /* 26 */
    TkMacOSXResizable, /* 27 */
    TkMacOSXSetHelpMenuItemCount, /* 28 */
    TkMacOSXSetScrollbarGrow, /* 29 */
    TkMacOSXSetUpClippingRgn, /* 30 */
    TkMacOSXSetUpGraphicsPort, /* 31 */
    TkMacOSXUpdateClipRgn, /* 32 */
    TkMacOSXUnregisterMacWindow, /* 33 */
    TkMacOSXUseMenuID, /* 34 */
    TkMacOSXVisableClipRgn, /* 35 */
    TkMacOSXWinBounds, /* 36 */
    TkMacOSXWindowOffset, /* 37 */
    TkSetMacColor, /* 38 */
    TkSetWMName, /* 39 */
    0, /* 40 */
    TkMacOSXZoomToplevel, /* 41 */
    Tk_TopCoordsToWindow, /* 42 */
    TkMacOSXContainerId, /* 43 */
    TkMacOSXGetHostToplevel, /* 44 */
    TkMacOSXPreprocessMenu, /* 45 */
    TkpIsWindowFloating, /* 46 */
    TkMacOSXGetCapture, /* 47 */
    0, /* 48 */
    TkGetTransientMaster, /* 49 */
    TkGenerateButtonEvent, /* 50 */
    TkGenWMDestroyEvent, /* 51 */
    TkMacOSXSetDrawingEnabled, /* 52 */
    TkpGetMS, /* 53 */
    TkMacOSXDrawable, /* 54 */
    TkpScanWindowId, /* 55 */
#endif /* AQUA */
#if !(defined(_WIN32) || defined(__CYGWIN__) || defined(MAC_OSX_TK)) /* X11 */
    TkCreateXEventSource, /* 0 */
    0, /* 1 */
    0, /* 2 */
    TkpCmapStressed, /* 3 */
    TkpSync, /* 4 */
    TkUnixContainerId, /* 5 */
    TkUnixDoOneXEvent, /* 6 */
    TkUnixSetMenubar, /* 7 */
    TkpScanWindowId, /* 8 */
    TkWmCleanup, /* 9 */
    TkSendCleanup, /* 10 */
    0, /* 11 */
    TkpWmSetState, /* 12 */
    TkpTestsendCmd, /* 13 */
#endif /* X11 */
};

static const TkIntXlibStubs tkIntXlibStubs = {
    TCL_STUB_MAGIC,
    0,
#if defined(_WIN32) || defined(__CYGWIN__) /* WIN */
    XSetDashes, /* 0 */
    XGetModifierMapping, /* 1 */
    XCreateImage, /* 2 */
    XGetImage, /* 3 */
    XGetAtomName, /* 4 */
    XKeysymToString, /* 5 */
    XCreateColormap, /* 6 */
    XCreatePixmapCursor, /* 7 */
    XCreateGlyphCursor, /* 8 */
    XGContextFromGC, /* 9 */
    XListHosts, /* 10 */
    XKeycodeToKeysym, /* 11 */
    XStringToKeysym, /* 12 */
    XRootWindow, /* 13 */
    XSetErrorHandler, /* 14 */
    XIconifyWindow, /* 15 */
    XWithdrawWindow, /* 16 */
    XGetWMColormapWindows, /* 17 */
    XAllocColor, /* 18 */
    XBell, /* 19 */
    XChangeProperty, /* 20 */
    XChangeWindowAttributes, /* 21 */
    XClearWindow, /* 22 */
    XConfigureWindow, /* 23 */
    XCopyArea, /* 24 */
    XCopyPlane, /* 25 */
    XCreateBitmapFromData, /* 26 */
    XDefineCursor, /* 27 */
    XDeleteProperty, /* 28 */
    XDestroyWindow, /* 29 */
    XDrawArc, /* 30 */
    XDrawLines, /* 31 */
    XDrawRectangle, /* 32 */
    XFillArc, /* 33 */
    XFillPolygon, /* 34 */
    XFillRectangles, /* 35 */
    XForceScreenSaver, /* 36 */
    XFreeColormap, /* 37 */
    XFreeColors, /* 38 */
    XFreeCursor, /* 39 */
    XFreeModifiermap, /* 40 */
    XGetGeometry, /* 41 */
    XGetInputFocus, /* 42 */
    XGetWindowProperty, /* 43 */
    XGetWindowAttributes, /* 44 */
    XGrabKeyboard, /* 45 */
    XGrabPointer, /* 46 */
    XKeysymToKeycode, /* 47 */
    XLookupColor, /* 48 */
    XMapWindow, /* 49 */
    XMoveResizeWindow, /* 50 */
    XMoveWindow, /* 51 */
    XNextEvent, /* 52 */
    XPutBackEvent, /* 53 */
    XQueryColors, /* 54 */
    XQueryPointer, /* 55 */
    XQueryTree, /* 56 */
    XRaiseWindow, /* 57 */
    XRefreshKeyboardMapping, /* 58 */
    XResizeWindow, /* 59 */
    XSelectInput, /* 60 */
    XSendEvent, /* 61 */
    XSetCommand, /* 62 */
    XSetIconName, /* 63 */
    XSetInputFocus, /* 64 */
    XSetSelectionOwner, /* 65 */
    XSetWindowBackground, /* 66 */
    XSetWindowBackgroundPixmap, /* 67 */
    XSetWindowBorder, /* 68 */
    XSetWindowBorderPixmap, /* 69 */
    XSetWindowBorderWidth, /* 70 */
    XSetWindowColormap, /* 71 */
    XTranslateCoordinates, /* 72 */
    XUngrabKeyboard, /* 73 */
    XUngrabPointer, /* 74 */
    XUnmapWindow, /* 75 */
    XWindowEvent, /* 76 */
    XDestroyIC, /* 77 */
    XFilterEvent, /* 78 */
    XmbLookupString, /* 79 */
    TkPutImage, /* 80 */
    0, /* 81 */
    XParseColor, /* 82 */
    XCreateGC, /* 83 */
    XFreeGC, /* 84 */
    XInternAtom, /* 85 */
    XSetBackground, /* 86 */
    XSetForeground, /* 87 */
    XSetClipMask, /* 88 */
    XSetClipOrigin, /* 89 */
    XSetTSOrigin, /* 90 */
    XChangeGC, /* 91 */
    XSetFont, /* 92 */
    XSetArcMode, /* 93 */
    XSetStipple, /* 94 */
    XSetFillRule, /* 95 */
    XSetFillStyle, /* 96 */
    XSetFunction, /* 97 */
    XSetLineAttributes, /* 98 */
    _XInitImageFuncPtrs, /* 99 */
    XCreateIC, /* 100 */
    XGetVisualInfo, /* 101 */
    XSetWMClientMachine, /* 102 */
    XStringListToTextProperty, /* 103 */
    XDrawLine, /* 104 */
    XWarpPointer, /* 105 */
    XFillRectangle, /* 106 */
    XFlush, /* 107 */
    XGrabServer, /* 108 */
    XUngrabServer, /* 109 */
    XFree, /* 110 */
    XNoOp, /* 111 */
    XSynchronize, /* 112 */
    XSync, /* 113 */
    XVisualIDFromVisual, /* 114 */
    0, /* 115 */
    0, /* 116 */
    0, /* 117 */
    0, /* 118 */
    0, /* 119 */
    XOffsetRegion, /* 120 */
    XUnionRegion, /* 121 */
    XCreateWindow, /* 122 */
    0, /* 123 */
    0, /* 124 */
    0, /* 125 */
    0, /* 126 */
    0, /* 127 */
    0, /* 128 */
    XLowerWindow, /* 129 */
    XFillArcs, /* 130 */
    XDrawArcs, /* 131 */
    XDrawRectangles, /* 132 */
    XDrawSegments, /* 133 */
    XDrawPoint, /* 134 */
    XDrawPoints, /* 135 */
    XReparentWindow, /* 136 */
    XPutImage, /* 137 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    XSetDashes, /* 0 */
    XGetModifierMapping, /* 1 */
    XCreateImage, /* 2 */
    XGetImage, /* 3 */
    XGetAtomName, /* 4 */
    XKeysymToString, /* 5 */
    XCreateColormap, /* 6 */
    XGContextFromGC, /* 7 */
    XKeycodeToKeysym, /* 8 */
    XStringToKeysym, /* 9 */
    XRootWindow, /* 10 */
    XSetErrorHandler, /* 11 */
    XAllocColor, /* 12 */
    XBell, /* 13 */
    XChangeProperty, /* 14 */
    XChangeWindowAttributes, /* 15 */
    XConfigureWindow, /* 16 */
    XCopyArea, /* 17 */
    XCopyPlane, /* 18 */
    XCreateBitmapFromData, /* 19 */
    XDefineCursor, /* 20 */
    XDestroyWindow, /* 21 */
    XDrawArc, /* 22 */
    XDrawLines, /* 23 */
    XDrawRectangle, /* 24 */
    XFillArc, /* 25 */
    XFillPolygon, /* 26 */
    XFillRectangles, /* 27 */
    XFreeColormap, /* 28 */
    XFreeColors, /* 29 */
    XFreeModifiermap, /* 30 */
    XGetGeometry, /* 31 */
    XGetWindowProperty, /* 32 */
    XGrabKeyboard, /* 33 */
    XGrabPointer, /* 34 */
    XKeysymToKeycode, /* 35 */
    XMapWindow, /* 36 */
    XMoveResizeWindow, /* 37 */
    XMoveWindow, /* 38 */
    XQueryPointer, /* 39 */
    XRaiseWindow, /* 40 */
    XRefreshKeyboardMapping, /* 41 */
    XResizeWindow, /* 42 */
    XSelectInput, /* 43 */
    XSendEvent, /* 44 */
    XSetIconName, /* 45 */
    XSetInputFocus, /* 46 */
    XSetSelectionOwner, /* 47 */
    XSetWindowBackground, /* 48 */
    XSetWindowBackgroundPixmap, /* 49 */
    XSetWindowBorder, /* 50 */
    XSetWindowBorderPixmap, /* 51 */
    XSetWindowBorderWidth, /* 52 */
    XSetWindowColormap, /* 53 */
    XUngrabKeyboard, /* 54 */
    XUngrabPointer, /* 55 */
    XUnmapWindow, /* 56 */
    TkPutImage, /* 57 */
    XParseColor, /* 58 */
    XCreateGC, /* 59 */
    XFreeGC, /* 60 */
    XInternAtom, /* 61 */
    XSetBackground, /* 62 */
    XSetForeground, /* 63 */
    XSetClipMask, /* 64 */
    XSetClipOrigin, /* 65 */
    XSetTSOrigin, /* 66 */
    XChangeGC, /* 67 */
    XSetFont, /* 68 */
    XSetArcMode, /* 69 */
    XSetStipple, /* 70 */
    XSetFillRule, /* 71 */
    XSetFillStyle, /* 72 */
    XSetFunction, /* 73 */
    XSetLineAttributes, /* 74 */
    _XInitImageFuncPtrs, /* 75 */
    XCreateIC, /* 76 */
    XGetVisualInfo, /* 77 */
    XSetWMClientMachine, /* 78 */
    XStringListToTextProperty, /* 79 */
    XDrawSegments, /* 80 */
    XForceScreenSaver, /* 81 */
    XDrawLine, /* 82 */
    XFillRectangle, /* 83 */
    XClearWindow, /* 84 */
    XDrawPoint, /* 85 */
    XDrawPoints, /* 86 */
    XWarpPointer, /* 87 */
    XQueryColor, /* 88 */
    XQueryColors, /* 89 */
    XQueryTree, /* 90 */
    XSync, /* 91 */
    0, /* 92 */
    0, /* 93 */
    0, /* 94 */
    0, /* 95 */
    0, /* 96 */
    0, /* 97 */
    0, /* 98 */
    0, /* 99 */
    0, /* 100 */
    0, /* 101 */
    0, /* 102 */
    0, /* 103 */
    0, /* 104 */
    0, /* 105 */
    0, /* 106 */
    XFlush, /* 107 */
    XGrabServer, /* 108 */
    XUngrabServer, /* 109 */
    XFree, /* 110 */
    XNoOp, /* 111 */
    XSynchronize, /* 112 */
    0, /* 113 */
    XVisualIDFromVisual, /* 114 */
    0, /* 115 */
    0, /* 116 */
    0, /* 117 */
    0, /* 118 */
    0, /* 119 */
    0, /* 120 */
    0, /* 121 */
    0, /* 122 */
    0, /* 123 */
    0, /* 124 */
    0, /* 125 */
    0, /* 126 */
    0, /* 127 */
    0, /* 128 */
    0, /* 129 */
    0, /* 130 */
    0, /* 131 */
    0, /* 132 */
    0, /* 133 */
    0, /* 134 */
    0, /* 135 */
    0, /* 136 */
    XPutImage, /* 137 */
#endif /* AQUA */
};

static const TkPlatStubs tkPlatStubs = {
    TCL_STUB_MAGIC,
    0,
#if defined(_WIN32) || defined(__CYGWIN__) /* WIN */
    Tk_AttachHWND, /* 0 */
    Tk_GetHINSTANCE, /* 1 */
    Tk_GetHWND, /* 2 */
    Tk_HWNDToWindow, /* 3 */
    Tk_PointerEvent, /* 4 */
    Tk_TranslateWinEvent, /* 5 */
#endif /* WIN */
#ifdef MAC_OSX_TK /* AQUA */
    Tk_MacOSXSetEmbedHandler, /* 0 */
    Tk_MacOSXTurnOffMenus, /* 1 */
    Tk_MacOSXTkOwnsCursor, /* 2 */
    TkMacOSXInitMenus, /* 3 */
    TkMacOSXInitAppleEvents, /* 4 */
    TkGenWMConfigureEvent, /* 5 */
    TkMacOSXInvalClipRgns, /* 6 */
    TkMacOSXGetDrawablePort, /* 7 */
    TkMacOSXGetRootControl, /* 8 */
    Tk_MacOSXSetupTkNotifier, /* 9 */
    Tk_MacOSXIsAppInFront, /* 10 */
#endif /* AQUA */
};

static const TkStubHooks tkStubHooks = {
    &tkPlatStubs,
    &tkIntStubs,
    &tkIntPlatStubs,
    &tkIntXlibStubs
};

const TkStubs tkStubs = {
    TCL_STUB_MAGIC,
    &tkStubHooks,
    Tk_MainLoop, /* 0 */
    Tk_3DBorderColor, /* 1 */
    Tk_3DBorderGC, /* 2 */
    Tk_3DHorizontalBevel, /* 3 */
    Tk_3DVerticalBevel, /* 4 */
    Tk_AddOption, /* 5 */
    Tk_BindEvent, /* 6 */
    Tk_CanvasDrawableCoords, /* 7 */
    Tk_CanvasEventuallyRedraw, /* 8 */
    Tk_CanvasGetCoord, /* 9 */
    Tk_CanvasGetTextInfo, /* 10 */
    Tk_CanvasPsBitmap, /* 11 */
    Tk_CanvasPsColor, /* 12 */
    Tk_CanvasPsFont, /* 13 */
    Tk_CanvasPsPath, /* 14 */
    Tk_CanvasPsStipple, /* 15 */
    Tk_CanvasPsY, /* 16 */
    Tk_CanvasSetStippleOrigin, /* 17 */
    Tk_CanvasTagsParseProc, /* 18 */
    Tk_CanvasTagsPrintProc, /* 19 */
    Tk_CanvasTkwin, /* 20 */
    Tk_CanvasWindowCoords, /* 21 */
    Tk_ChangeWindowAttributes, /* 22 */
    Tk_CharBbox, /* 23 */
    Tk_ClearSelection, /* 24 */
    Tk_ClipboardAppend, /* 25 */
    Tk_ClipboardClear, /* 26 */
    Tk_ConfigureInfo, /* 27 */
    Tk_ConfigureValue, /* 28 */
    Tk_ConfigureWidget, /* 29 */
    Tk_ConfigureWindow, /* 30 */
    Tk_ComputeTextLayout, /* 31 */
    Tk_CoordsToWindow, /* 32 */
    Tk_CreateBinding, /* 33 */
    Tk_CreateBindingTable, /* 34 */
    Tk_CreateErrorHandler, /* 35 */
    Tk_CreateEventHandler, /* 36 */
    Tk_CreateGenericHandler, /* 37 */
    Tk_CreateImageType, /* 38 */
    Tk_CreateItemType, /* 39 */
    Tk_CreatePhotoImageFormat, /* 40 */
    Tk_CreateSelHandler, /* 41 */
    Tk_CreateWindow, /* 42 */
    Tk_CreateWindowFromPath, /* 43 */
    Tk_DefineBitmap, /* 44 */
    Tk_DefineCursor, /* 45 */
    Tk_DeleteAllBindings, /* 46 */
    Tk_DeleteBinding, /* 47 */
    Tk_DeleteBindingTable, /* 48 */
    Tk_DeleteErrorHandler, /* 49 */
    Tk_DeleteEventHandler, /* 50 */
    Tk_DeleteGenericHandler, /* 51 */
    Tk_DeleteImage, /* 52 */
    Tk_DeleteSelHandler, /* 53 */
    Tk_DestroyWindow, /* 54 */
    Tk_DisplayName, /* 55 */
    Tk_DistanceToTextLayout, /* 56 */
    Tk_Draw3DPolygon, /* 57 */
    Tk_Draw3DRectangle, /* 58 */
    Tk_DrawChars, /* 59 */
    Tk_DrawFocusHighlight, /* 60 */
    Tk_DrawTextLayout, /* 61 */
    Tk_Fill3DPolygon, /* 62 */
    Tk_Fill3DRectangle, /* 63 */
    Tk_FindPhoto, /* 64 */
    Tk_FontId, /* 65 */
    Tk_Free3DBorder, /* 66 */
    Tk_FreeBitmap, /* 67 */
    Tk_FreeColor, /* 68 */
    Tk_FreeColormap, /* 69 */
    Tk_FreeCursor, /* 70 */
    Tk_FreeFont, /* 71 */
    Tk_FreeGC, /* 72 */
    Tk_FreeImage, /* 73 */
    Tk_FreeOptions, /* 74 */
    Tk_FreePixmap, /* 75 */
    Tk_FreeTextLayout, /* 76 */
    Tk_FreeXId, /* 77 */
    Tk_GCForColor, /* 78 */
    Tk_GeometryRequest, /* 79 */
    Tk_Get3DBorder, /* 80 */
    Tk_GetAllBindings, /* 81 */
    Tk_GetAnchor, /* 82 */
    Tk_GetAtomName, /* 83 */
    Tk_GetBinding, /* 84 */
    Tk_GetBitmap, /* 85 */
    Tk_GetBitmapFromData, /* 86 */
    Tk_GetCapStyle, /* 87 */
    Tk_GetColor, /* 88 */
    Tk_GetColorByValue, /* 89 */
    Tk_GetColormap, /* 90 */
    Tk_GetCursor, /* 91 */
    Tk_GetCursorFromData, /* 92 */
    Tk_GetFont, /* 93 */
    Tk_GetFontFromObj, /* 94 */
    Tk_GetFontMetrics, /* 95 */
    Tk_GetGC, /* 96 */
    Tk_GetImage, /* 97 */
    Tk_GetImageMasterData, /* 98 */
    Tk_GetItemTypes, /* 99 */
    Tk_GetJoinStyle, /* 100 */
    Tk_GetJustify, /* 101 */
    Tk_GetNumMainWindows, /* 102 */
    Tk_GetOption, /* 103 */
    Tk_GetPixels, /* 104 */
    Tk_GetPixmap, /* 105 */
    Tk_GetRelief, /* 106 */
    Tk_GetRootCoords, /* 107 */
    Tk_GetScrollInfo, /* 108 */
    Tk_GetScreenMM, /* 109 */
    Tk_GetSelection, /* 110 */
    Tk_GetUid, /* 111 */
    Tk_GetVisual, /* 112 */
    Tk_GetVRootGeometry, /* 113 */
    Tk_Grab, /* 114 */
    Tk_HandleEvent, /* 115 */
    Tk_IdToWindow, /* 116 */
    Tk_ImageChanged, /* 117 */
    Tk_Init, /* 118 */
    Tk_InternAtom, /* 119 */
    Tk_IntersectTextLayout, /* 120 */
    Tk_MaintainGeometry, /* 121 */
    Tk_MainWindow, /* 122 */
    Tk_MakeWindowExist, /* 123 */
    Tk_ManageGeometry, /* 124 */
    Tk_MapWindow, /* 125 */
    Tk_MeasureChars, /* 126 */
    Tk_MoveResizeWindow, /* 127 */
    Tk_MoveWindow, /* 128 */
    Tk_MoveToplevelWindow, /* 129 */
    Tk_NameOf3DBorder, /* 130 */
    Tk_NameOfAnchor, /* 131 */
    Tk_NameOfBitmap, /* 132 */
    Tk_NameOfCapStyle, /* 133 */
    Tk_NameOfColor, /* 134 */
    Tk_NameOfCursor, /* 135 */
    Tk_NameOfFont, /* 136 */
    Tk_NameOfImage, /* 137 */
    Tk_NameOfJoinStyle, /* 138 */
    Tk_NameOfJustify, /* 139 */
    Tk_NameOfRelief, /* 140 */
    Tk_NameToWindow, /* 141 */
    Tk_OwnSelection, /* 142 */
    Tk_ParseArgv, /* 143 */
    Tk_PhotoPutBlock_NoComposite, /* 144 */
    Tk_PhotoPutZoomedBlock_NoComposite, /* 145 */
    Tk_PhotoGetImage, /* 146 */
    Tk_PhotoBlank, /* 147 */
    Tk_PhotoExpand_Panic, /* 148 */
    Tk_PhotoGetSize, /* 149 */
    Tk_PhotoSetSize_Panic, /* 150 */
    Tk_PointToChar, /* 151 */
    Tk_PostscriptFontName, /* 152 */
    Tk_PreserveColormap, /* 153 */
    Tk_QueueWindowEvent, /* 154 */
    Tk_RedrawImage, /* 155 */
    Tk_ResizeWindow, /* 156 */
    Tk_RestackWindow, /* 157 */
    Tk_RestrictEvents, /* 158 */
    Tk_SafeInit, /* 159 */
    Tk_SetAppName, /* 160 */
    Tk_SetBackgroundFromBorder, /* 161 */
    Tk_SetClass, /* 162 */
    Tk_SetGrid, /* 163 */
    Tk_SetInternalBorder, /* 164 */
    Tk_SetWindowBackground, /* 165 */
    Tk_SetWindowBackgroundPixmap, /* 166 */
    Tk_SetWindowBorder, /* 167 */
    Tk_SetWindowBorderWidth, /* 168 */
    Tk_SetWindowBorderPixmap, /* 169 */
    Tk_SetWindowColormap, /* 170 */
    Tk_SetWindowVisual, /* 171 */
    Tk_SizeOfBitmap, /* 172 */
    Tk_SizeOfImage, /* 173 */
    Tk_StrictMotif, /* 174 */
    Tk_TextLayoutToPostscript, /* 175 */
    Tk_TextWidth, /* 176 */
    Tk_UndefineCursor, /* 177 */
    Tk_UnderlineChars, /* 178 */
    Tk_UnderlineTextLayout, /* 179 */
    Tk_Ungrab, /* 180 */
    Tk_UnmaintainGeometry, /* 181 */
    Tk_UnmapWindow, /* 182 */
    Tk_UnsetGrid, /* 183 */
    Tk_UpdatePointer, /* 184 */
    Tk_AllocBitmapFromObj, /* 185 */
    Tk_Alloc3DBorderFromObj, /* 186 */
    Tk_AllocColorFromObj, /* 187 */
    Tk_AllocCursorFromObj, /* 188 */
    Tk_AllocFontFromObj, /* 189 */
    Tk_CreateOptionTable, /* 190 */
    Tk_DeleteOptionTable, /* 191 */
    Tk_Free3DBorderFromObj, /* 192 */
    Tk_FreeBitmapFromObj, /* 193 */
    Tk_FreeColorFromObj, /* 194 */
    Tk_FreeConfigOptions, /* 195 */
    Tk_FreeSavedOptions, /* 196 */
    Tk_FreeCursorFromObj, /* 197 */
    Tk_FreeFontFromObj, /* 198 */
    Tk_Get3DBorderFromObj, /* 199 */
    Tk_GetAnchorFromObj, /* 200 */
    Tk_GetBitmapFromObj, /* 201 */
    Tk_GetColorFromObj, /* 202 */
    Tk_GetCursorFromObj, /* 203 */
    Tk_GetOptionInfo, /* 204 */
    Tk_GetOptionValue, /* 205 */
    Tk_GetJustifyFromObj, /* 206 */
    Tk_GetMMFromObj, /* 207 */
    Tk_GetPixelsFromObj, /* 208 */
    Tk_GetReliefFromObj, /* 209 */
    Tk_GetScrollInfoObj, /* 210 */
    Tk_InitOptions, /* 211 */
    Tk_MainEx, /* 212 */
    Tk_RestoreSavedOptions, /* 213 */
    Tk_SetOptions, /* 214 */
    Tk_InitConsoleChannels, /* 215 */
    Tk_CreateConsoleWindow, /* 216 */
    Tk_CreateSmoothMethod, /* 217 */
    0, /* 218 */
    0, /* 219 */
    Tk_GetDash, /* 220 */
    Tk_CreateOutline, /* 221 */
    Tk_DeleteOutline, /* 222 */
    Tk_ConfigOutlineGC, /* 223 */
    Tk_ChangeOutlineGC, /* 224 */
    Tk_ResetOutlineGC, /* 225 */
    Tk_CanvasPsOutline, /* 226 */
    Tk_SetTSOrigin, /* 227 */
    Tk_CanvasGetCoordFromObj, /* 228 */
    Tk_CanvasSetOffset, /* 229 */
    Tk_DitherPhoto, /* 230 */
    Tk_PostscriptBitmap, /* 231 */
    Tk_PostscriptColor, /* 232 */
    Tk_PostscriptFont, /* 233 */
    Tk_PostscriptImage, /* 234 */
    Tk_PostscriptPath, /* 235 */
    Tk_PostscriptStipple, /* 236 */
    Tk_PostscriptY, /* 237 */
    Tk_PostscriptPhoto, /* 238 */
    Tk_CreateClientMessageHandler, /* 239 */
    Tk_DeleteClientMessageHandler, /* 240 */
    Tk_CreateAnonymousWindow, /* 241 */
    Tk_SetClassProcs, /* 242 */
    Tk_SetInternalBorderEx, /* 243 */
    Tk_SetMinimumRequestSize, /* 244 */
    Tk_SetCaretPos, /* 245 */
    Tk_PhotoPutBlock_Panic, /* 246 */
    Tk_PhotoPutZoomedBlock_Panic, /* 247 */
    Tk_CollapseMotionEvents, /* 248 */
    Tk_RegisterStyleEngine, /* 249 */
    Tk_GetStyleEngine, /* 250 */
    Tk_RegisterStyledElement, /* 251 */
    Tk_GetElementId, /* 252 */
    Tk_CreateStyle, /* 253 */
    Tk_GetStyle, /* 254 */
    Tk_FreeStyle, /* 255 */
    Tk_NameOfStyle, /* 256 */
    Tk_AllocStyleFromObj, /* 257 */
    Tk_GetStyleFromObj, /* 258 */
    Tk_FreeStyleFromObj, /* 259 */
    Tk_GetStyledElement, /* 260 */
    Tk_GetElementSize, /* 261 */
    Tk_GetElementBox, /* 262 */
    Tk_GetElementBorderWidth, /* 263 */
    Tk_DrawElement, /* 264 */
    Tk_PhotoExpand, /* 265 */
    Tk_PhotoPutBlock, /* 266 */
    Tk_PhotoPutZoomedBlock, /* 267 */
    Tk_PhotoSetSize, /* 268 */
    Tk_GetUserInactiveTime, /* 269 */
    Tk_ResetUserInactiveTime, /* 270 */
    Tk_Interp, /* 271 */
    Tk_CreateOldImageType, /* 272 */
    Tk_CreateOldPhotoImageFormat, /* 273 */
};

/* !END!: Do not edit above this line. */
