/*
 * tkWinX.c --
 *
 *	This file contains Windows emulation procedures for X routines.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 * Copyright (c) 1994 Software Research Associates, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkWinInt.h"

/*
 * The w32api 1.1 package (included in Mingw 1.1) does not define _WIN32_IE by
 * default. Define it here to gain access to the InitCommonControlsEx API in
 * commctrl.h.
 */

#ifndef _WIN32_IE
#define _WIN32_IE 0x0550 /* IE 5.5 */
#endif

#include <commctrl.h>
#ifdef _MSC_VER
#   pragma comment (lib, "comctl32.lib")
#   pragma comment (lib, "advapi32.lib")
#endif

/*
 * The zmouse.h file includes the definition for WM_MOUSEWHEEL.
 */

#include <zmouse.h>

/*
 * WM_MOUSEHWHEEL is normally defined by Winuser.h for Vista/2008 or later,
 * but is also usable on 2000/XP if IntelliPoint drivers are installed.
 */

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

/*
 * imm.h is needed by HandleIMEComposition
 */

#include <imm.h>
#ifdef _MSC_VER
#   pragma comment (lib, "imm32.lib")
#endif

/*
 * WM_UNICHAR is a message for Unicode input on all windows systems.
 * Perhaps this definition should be moved in another file.
 */
#ifndef WM_UNICHAR
#define WM_UNICHAR     0x0109
#define UNICODE_NOCHAR 0xFFFF
#endif

/*
 * Declarations of static variables used in this file.
 */

static const char winScreenName[] = ":0"; /* Default name of windows display. */
static HINSTANCE tkInstance = NULL;	/* Application instance handle. */
static int childClassInitialized;	/* Registered child class? */
static WNDCLASSW childClass;		/* Window class for child windows. */
static int tkPlatformId = 0;		/* version of Windows platform */
static int tkWinTheme = 0;		/* See TkWinGetPlatformTheme */
static Tcl_Encoding keyInputEncoding = NULL;
					/* The current character encoding for
					 * keyboard input */
static int keyInputCharset = -1;	/* The Win32 CHARSET for the keyboard
					 * encoding */
static Tcl_Encoding unicodeEncoding = NULL;
					/* The UNICODE encoding */

/*
 * Thread local storage. Notice that now each thread must have its own
 * TkDisplay structure, since this structure contains most of the thread-
 * specific date for threads.
 */

typedef struct {
    TkDisplay *winDisplay;	/* TkDisplay structure that represents Windows
				 * screen. */
    int updatingClipboard;	/* If 1, we are updating the clipboard. */
    int surrogateBuffer;	/* Buffer for first of surrogate pair. */
    DWORD vWheelTickPrev;	/* For high resolution wheels (vertical). */
    DWORD hWheelTickPrev;	/* For high resolution wheels (horizontal). */
    short vWheelAcc;		/* For high resolution wheels (vertical). */
    short hWheelAcc;		/* For high resolution wheels (horizontal). */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations of functions used in this file.
 */

static void		GenerateXEvent(HWND hwnd, UINT message,
			    WPARAM wParam, LPARAM lParam);
static unsigned int	GetState(UINT message, WPARAM wParam, LPARAM lParam);
static void 		GetTranslatedKey(XKeyEvent *xkey, UINT type);
static void		UpdateInputLanguage(int charset);
static int		HandleIMEComposition(HWND hwnd, LPARAM lParam);

/*
 *----------------------------------------------------------------------
 *
 * TkGetServerInfo --
 *
 *	Given a window, this function returns information about the window
 *	server for that window. This function provides the guts of the "winfo
 *	server" command.
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
TkGetServerInfo(
    Tcl_Interp *interp,		/* The server information is returned in this
				 * interpreter's result. */
    Tk_Window tkwin)		/* Token for window; this selects a particular
				 * display and server. */
{
    static char buffer[32]; /* Empty string means not initialized yet. */
    OSVERSIONINFOW os;

    if (!buffer[0]) {
	HANDLE handle = GetModuleHandleW(L"NTDLL");
	int(__stdcall *getversion)(void *) =
		(int(__stdcall *)(void *))GetProcAddress(handle, "RtlGetVersion");
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	if (!getversion || getversion(&os)) {
	    GetVersionExW(&os);
	}
	/* Write the first character last, preventing multi-thread issues. */
	sprintf(buffer+1, "indows %d.%d %d %s", (int)os.dwMajorVersion,
		(int)os.dwMinorVersion, (int)os.dwBuildNumber,
#ifdef _WIN64
		"Win64"
#else
		"Win32"
#endif
	);
	buffer[0] = 'W';
    }
    Tcl_AppendResult(interp, buffer, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetHINSTANCE --
 *
 *	Retrieves the global instance handle used by the Tk library.
 *
 * Results:
 *	Returns the global instance handle.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

HINSTANCE
Tk_GetHINSTANCE(void)
{
    if (tkInstance == NULL) {
	tkInstance = GetModuleHandleW(NULL);
    }
    return tkInstance;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinSetHINSTANCE --
 *
 *	Sets the global instance handle used by the Tk library. This should be
 *	called by DllMain.
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
TkWinSetHINSTANCE(
    HINSTANCE hInstance)
{
    tkInstance = hInstance;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinXInit --
 *
 *	Initialize Xlib emulation layer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up various data structures.
 *
 *----------------------------------------------------------------------
 */

void
TkWinXInit(
    HINSTANCE hInstance)
{
    INITCOMMONCONTROLSEX comctl;
    CHARSETINFO lpCs;
    DWORD lpCP;

    if (childClassInitialized != 0) {
	return;
    }
    childClassInitialized = 1;

    comctl.dwSize = sizeof(INITCOMMONCONTROLSEX);
    comctl.dwICC = ICC_WIN95_CLASSES;
    if (!InitCommonControlsEx(&comctl)) {
	Tcl_Panic("Unable to load common controls?!");
    }

    childClass.style = CS_HREDRAW | CS_VREDRAW;
    childClass.cbClsExtra = 0;
    childClass.cbWndExtra = 0;
    childClass.hInstance = hInstance;
    childClass.hbrBackground = NULL;
    childClass.lpszMenuName = NULL;

    /*
     * Register the Child window class.
     */

    childClass.lpszClassName = TK_WIN_CHILD_CLASS_NAME;
    childClass.lpfnWndProc = TkWinChildProc;
    childClass.hIcon = NULL;
    childClass.hCursor = NULL;

    if (!RegisterClassW(&childClass)) {
	Tcl_Panic("Unable to register TkChild class");
    }

    /*
     * Initialize input language info
     */

    if (GetLocaleInfoW(LANGIDFROMLCID(PTR2INT(GetKeyboardLayout(0))),
	       LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
	       (LPWSTR) &lpCP, sizeof(lpCP)/sizeof(WCHAR))
	    && TranslateCharsetInfo(INT2PTR(lpCP), &lpCs, TCI_SRCCODEPAGE)) {
	UpdateInputLanguage((int) lpCs.ciCharset);
    }

    /*
     * Make sure we cleanup on finalize.
     */

    TkCreateExitHandler(TkWinXCleanup, (ClientData) hInstance);
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinXCleanup --
 *
 *	Removes the registered classes for Tk.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Removes window classes from the system.
 *
 *----------------------------------------------------------------------
 */

void
TkWinXCleanup(
    ClientData clientData)
{
    HINSTANCE hInstance = (HINSTANCE) clientData;

    /*
     * Clean up our own class.
     */

    if (childClassInitialized) {
	childClassInitialized = 0;
	UnregisterClassW(TK_WIN_CHILD_CLASS_NAME, hInstance);
    }

    if (unicodeEncoding != NULL) {
	Tcl_FreeEncoding(unicodeEncoding);
	unicodeEncoding = NULL;
    }

    /*
     * And let the window manager clean up its own class(es).
     */

    TkWinWmCleanup(hInstance);
    TkWinCleanupContainerList();
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetPlatformId --
 *
 *	Determines whether running under NT, 95, or Win32s, to allow runtime
 *	conditional code. Win32s is no longer supported.
 *
 * Results:
 *	The return value is one of:
 *		VER_PLATFORM_WIN32s	   Win32s on Windows 3.1 (not supported)
 *		VER_PLATFORM_WIN32_WINDOWS Win32 on Windows 95, 98, ME (not supported)
 *		VER_PLATFORM_WIN32_NT	Win32 on Windows XP, Vista, Windows 7, Windows 8
 *		VER_PLATFORM_WIN32_CE	Win32 on Windows CE
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkWinGetPlatformId(void)
{
    if (tkPlatformId == 0) {
	OSVERSIONINFOW os;

	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
	GetVersionExW(&os);
	tkPlatformId = os.dwPlatformId;

	/*
	 * Set tkWinTheme to be TK_THEME_WIN_XP or TK_THEME_WIN_CLASSIC. The
	 * TK_THEME_WIN_CLASSIC could be set even when running under XP if the
	 * windows classic theme was selected.
	 */

	if ((os.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
		(os.dwMajorVersion == 5 && os.dwMinorVersion == 1)) {
	    HKEY hKey;
	    LPCWSTR szSubKey = L"Control Panel\\Appearance";
	    LPCWSTR szCurrent = L"Current";
	    DWORD dwSize = 200;
	    char pBuffer[200];

	    memset(pBuffer, 0, dwSize);
	    if (RegOpenKeyExW(HKEY_CURRENT_USER, szSubKey, 0L,
		    KEY_READ, &hKey) != ERROR_SUCCESS) {
		tkWinTheme = TK_THEME_WIN_XP;
	    } else {
		RegQueryValueExW(hKey, szCurrent, NULL, NULL, (LPBYTE) pBuffer, &dwSize);
		RegCloseKey(hKey);
		if (strcmp(pBuffer, "Windows Standard") == 0) {
		    tkWinTheme = TK_THEME_WIN_CLASSIC;
		} else {
		    tkWinTheme = TK_THEME_WIN_XP;
		}
	    }
	} else {
	    tkWinTheme = TK_THEME_WIN_CLASSIC;
	}
    }
    return tkPlatformId;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetPlatformTheme --
 *
 *	Return the Windows drawing style we should be using.
 *
 * Results:
 *	The return value is one of:
 *	    TK_THEME_WIN_CLASSIC	95/98/NT or XP in classic mode
 *	    TK_THEME_WIN_XP		XP not in classic mode
 *
 * Side effects:
 *	Could invoke TkWinGetPlatformId.
 *
 *----------------------------------------------------------------------
 */

int
TkWinGetPlatformTheme(void)
{
    if (tkPlatformId == 0) {
	TkWinGetPlatformId();
    }
    return tkWinTheme;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetDefaultScreenName --
 *
 *	Returns the name of the screen that Tk should use during
 *	initialization.
 *
 * Results:
 *	Returns a statically allocated string.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

const char *
TkGetDefaultScreenName(
    Tcl_Interp *interp,		/* Not used. */
    const char *screenName)	/* If NULL, use default string. */
{
    if ((screenName == NULL) || (screenName[0] == '\0')) {
	screenName = winScreenName;
    }
    return screenName;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinDisplayChanged --
 *
 *	Called to set up initial screen info or when an event indicated
 *	display (screen) change.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May change info regarding the screen.
 *
 *----------------------------------------------------------------------
 */

void
TkWinDisplayChanged(
    Display *display)
{
    HDC dc;
    Screen *screen;

    if (display == NULL || display->screens == NULL) {
	return;
    }
    screen = display->screens;

    dc = GetDC(NULL);
    screen->width = GetDeviceCaps(dc, HORZRES);
    screen->height = GetDeviceCaps(dc, VERTRES);
    screen->mwidth = MulDiv(screen->width, 254,
	    GetDeviceCaps(dc, LOGPIXELSX) * 10);
    screen->mheight = MulDiv(screen->height, 254,
	    GetDeviceCaps(dc, LOGPIXELSY) * 10);

    /*
     * On windows, when creating a color bitmap, need two pieces of
     * information: the number of color planes and the number of pixels per
     * plane. Need to remember both quantities so that when constructing an
     * HBITMAP for offscreen rendering, we can specify the correct value for
     * the number of planes. Otherwise the HBITMAP won't be compatible with
     * the HWND and we'll just get blank spots copied onto the screen.
     */

    screen->ext_data = INT2PTR(GetDeviceCaps(dc, PLANES));
    screen->root_depth = GetDeviceCaps(dc, BITSPIXEL) * PTR2INT(screen->ext_data);

    if (screen->root_visual != NULL) {
	ckfree(screen->root_visual);
    }
    screen->root_visual = ckalloc(sizeof(Visual));
    screen->root_visual->visualid = 0;
    if (GetDeviceCaps(dc, RASTERCAPS) & RC_PALETTE) {
	screen->root_visual->map_entries = GetDeviceCaps(dc, SIZEPALETTE);
	screen->root_visual->c_class = PseudoColor;
	screen->root_visual->red_mask = 0x0;
	screen->root_visual->green_mask = 0x0;
	screen->root_visual->blue_mask = 0x0;
    } else if (screen->root_depth == 4) {
	screen->root_visual->c_class = StaticColor;
	screen->root_visual->map_entries = 16;
    } else if (screen->root_depth == 8) {
	screen->root_visual->c_class = StaticColor;
	screen->root_visual->map_entries = 256;
    } else if (screen->root_depth == 12) {
	screen->root_visual->c_class = TrueColor;
	screen->root_visual->map_entries = 32;
	screen->root_visual->red_mask = 0xf0;
	screen->root_visual->green_mask = 0xf000;
	screen->root_visual->blue_mask = 0xf00000;
    } else if (screen->root_depth == 16) {
	screen->root_visual->c_class = TrueColor;
	screen->root_visual->map_entries = 64;
	screen->root_visual->red_mask = 0xf8;
	screen->root_visual->green_mask = 0xfc00;
	screen->root_visual->blue_mask = 0xf80000;
    } else if (screen->root_depth >= 24) {
	screen->root_visual->c_class = TrueColor;
	screen->root_visual->map_entries = 256;
	screen->root_visual->red_mask = 0xff;
	screen->root_visual->green_mask = 0xff00;
	screen->root_visual->blue_mask = 0xff0000;
    }
    screen->root_visual->bits_per_rgb = screen->root_depth;
    ReleaseDC(NULL, dc);

    if (screen->cmap != None) {
	XFreeColormap(display, screen->cmap);
    }
    screen->cmap = XCreateColormap(display, None, screen->root_visual,
	    AllocNone);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpOpenDisplay --
 *
 *	Create the Display structure and fill it with device specific
 *	information.
 *
 * Results:
 *	Returns a TkDisplay structure on success or NULL on failure.
 *
 * Side effects:
 *	Allocates a new TkDisplay structure.
 *
 *----------------------------------------------------------------------
 */

TkDisplay *
TkpOpenDisplay(
    const char *display_name)
{
    Screen *screen;
    TkWinDrawable *twdPtr;
    Display *display;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    DWORD initialWheelTick;

    if (tsdPtr->winDisplay != NULL) {
	if (!strcmp(tsdPtr->winDisplay->display->display_name, display_name)) {
	    return tsdPtr->winDisplay;
	} else {
	    return NULL;
	}
    }

    display = ckalloc(sizeof(Display));
    ZeroMemory(display, sizeof(Display));

    display->display_name = ckalloc(strlen(display_name) + 1);
    strcpy(display->display_name, display_name);

    display->cursor_font = 1;
    display->nscreens = 1;
    display->request = 1;
    display->qlen = 0;

    screen = ckalloc(sizeof(Screen));
    ZeroMemory(screen, sizeof(Screen));
    screen->display = display;

    /*
     * Set up the root window.
     */

    twdPtr = ckalloc(sizeof(TkWinDrawable));
    if (twdPtr == NULL) {
	return NULL;
    }
    twdPtr->type = TWD_WINDOW;
    twdPtr->window.winPtr = NULL;
    twdPtr->window.handle = NULL;
    screen->root = (Window)twdPtr;

    /*
     * Note that these pixel values are not palette relative.
     */

    screen->white_pixel = RGB(255, 255, 255);
    screen->black_pixel = RGB(0, 0, 0);
    screen->cmap = None;

    display->screens		= screen;
    display->nscreens		= 1;
    display->default_screen	= 0;

    TkWinDisplayChanged(display);

    tsdPtr->winDisplay = ckalloc(sizeof(TkDisplay));
    ZeroMemory(tsdPtr->winDisplay, sizeof(TkDisplay));
    tsdPtr->winDisplay->display = display;
    tsdPtr->updatingClipboard = FALSE;
    initialWheelTick = GetTickCount();
    tsdPtr->vWheelTickPrev = initialWheelTick;
    tsdPtr->hWheelTickPrev = initialWheelTick;
    tsdPtr->vWheelAcc = 0;
    tsdPtr->hWheelAcc = 0;

    /*
     * Key map info must be available immediately, because of "send event".
     */
    TkpInitKeymapInfo(tsdPtr->winDisplay);

    return tsdPtr->winDisplay;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCloseDisplay --
 *
 *	Closes and deallocates a Display structure created with the
 *	TkpOpenDisplay function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees up memory.
 *
 *----------------------------------------------------------------------
 */

void
TkpCloseDisplay(
    TkDisplay *dispPtr)
{
    Display *display = dispPtr->display;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (dispPtr != tsdPtr->winDisplay) {
	Tcl_Panic("TkpCloseDisplay: tried to call TkpCloseDisplay on another display");
	return; /* not reached */
    }

    tsdPtr->winDisplay = NULL;

    if (display->display_name != NULL) {
	ckfree(display->display_name);
    }
    if (display->screens != NULL) {
	if (display->screens->root_visual != NULL) {
	    ckfree(display->screens->root_visual);
	}
	if (display->screens->root != None) {
	    ckfree(display->screens->root);
	}
	if (display->screens->cmap != None) {
	    XFreeColormap(display, display->screens->cmap);
	}
	ckfree(display->screens);
    }
    ckfree(display);
}

/*
 *----------------------------------------------------------------------
 *
 * TkClipCleanup --
 *
 *	This function is called to cleanup resources associated with claiming
 *	clipboard ownership and for receiving selection get results. This
 *	function is called in tkWindow.c. This has to be called by the display
 *	cleanup function because we still need the access display elements.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Resources are freed - the clipboard may no longer be used.
 *
 *----------------------------------------------------------------------
 */

void
TkClipCleanup(
    TkDisplay *dispPtr)		/* Display associated with clipboard. */
{
    if (dispPtr->clipWindow != NULL) {
	Tk_DeleteSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
		dispPtr->applicationAtom);
	Tk_DeleteSelHandler(dispPtr->clipWindow, dispPtr->clipboardAtom,
		dispPtr->windowAtom);

	Tk_DestroyWindow(dispPtr->clipWindow);
	Tcl_Release((ClientData) dispPtr->clipWindow);
	dispPtr->clipWindow = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * XBell --
 *
 *	Generate a beep.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Plays a sounds out the system speakers.
 *
 *----------------------------------------------------------------------
 */

int
XBell(
    Display *display,
    int percent)
{
    MessageBeep(MB_OK);
    return Success;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinChildProc --
 *
 *	Callback from Windows whenever an event occurs on a child window.
 *
 * Results:
 *	Standard Windows return value.
 *
 * Side effects:
 *	May process events off the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

LRESULT CALLBACK
TkWinChildProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result;

    switch (message) {
    case WM_INPUTLANGCHANGE:
	UpdateInputLanguage((int) wParam);
	result = 1;
	break;

    case WM_IME_COMPOSITION:
	result = 0;
	if (HandleIMEComposition(hwnd, lParam) == 0) {
	    result = DefWindowProcW(hwnd, message, wParam, lParam);
	}
	break;

    case WM_SETCURSOR:
	/*
	 * Short circuit the WM_SETCURSOR message since we set the cursor
	 * elsewhere.
	 */

	result = TRUE;
	break;

    case WM_CREATE:
    case WM_ERASEBKGND:
	result = 0;
	break;

    case WM_PAINT:
	GenerateXEvent(hwnd, message, wParam, lParam);
	result = DefWindowProcW(hwnd, message, wParam, lParam);
	break;

    case TK_CLAIMFOCUS:
    case TK_GEOMETRYREQ:
    case TK_ATTACHWINDOW:
    case TK_DETACHWINDOW:
    case TK_ICONIFY:
    case TK_DEICONIFY:
    case TK_MOVEWINDOW:
    case TK_WITHDRAW:
    case TK_RAISEWINDOW:
    case TK_GETFRAMEWID:
    case TK_OVERRIDEREDIRECT:
    case TK_SETMENU:
    case TK_STATE:
    case TK_INFO:
	result = TkWinEmbeddedEventProc(hwnd, message, wParam, lParam);
	break;

    case WM_UNICHAR:
        if (wParam == UNICODE_NOCHAR) {
	    /* If wParam is UNICODE_NOCHAR and the application processes
	     * this message, then return TRUE. */
	    result = 1;
	} else {
	    /* If the event was translated, we must return 0 */
            if (Tk_TranslateWinEvent(hwnd, message, wParam, lParam, &result)) {
                result = 0;
	    } else {
	        result = 1;
	    }
	}
	break;

    default:
	if (!Tk_TranslateWinEvent(hwnd, message, wParam, lParam, &result)) {
	    result = DefWindowProcW(hwnd, message, wParam, lParam);
	}
	break;
    }

    /*
     * Handle any newly queued events before returning control to Windows.
     */

    Tcl_ServiceAll();
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_TranslateWinEvent --
 *
 *	This function is called by widget window functions to handle the
 *	translation from Win32 events to Tk events.
 *
 * Results:
 *	Returns 1 if the event was handled, else 0.
 *
 * Side effects:
 *	Depends on the event.
 *
 *----------------------------------------------------------------------
 */

int
Tk_TranslateWinEvent(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    LRESULT *resultPtr)
{
    *resultPtr = 0;
    switch (message) {
    case WM_RENDERFORMAT: {
	TkWindow *winPtr = (TkWindow *) Tk_HWNDToWindow(hwnd);

	if (winPtr) {
	    TkWinClipboardRender(winPtr->dispPtr, wParam);
	}
	return 1;
    }

    case WM_RENDERALLFORMATS: {
        TkWindow *winPtr = (TkWindow *) Tk_HWNDToWindow(hwnd);

        if (winPtr && OpenClipboard(hwnd)) {
            /*
             * Make sure that nobody had taken ownership of the clipboard
             * before we opened it.
             */

            if (GetClipboardOwner() == hwnd) {
                TkWinClipboardRender(winPtr->dispPtr, CF_TEXT);
            }
            CloseClipboard();
        }
        return 1;
    }

    case WM_COMMAND:
    case WM_NOTIFY:
    case WM_VSCROLL:
    case WM_HSCROLL: {
	/*
	 * Reflect these messages back to the sender so that they can be
	 * handled by the window proc for the control. Note that we need to be
	 * careful not to reflect a message that is targeted to this window,
	 * or we will loop.
	 */

	HWND target = (message == WM_NOTIFY)
		? ((NMHDR*)lParam)->hwndFrom : (HWND) lParam;

	if (target && target != hwnd) {
	    *resultPtr = SendMessageW(target, message, wParam, lParam);
	    return 1;
	}
	break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
    case WM_XBUTTONDOWN:
    case WM_XBUTTONDBLCLK:
    case WM_LBUTTONUP:
    case WM_MBUTTONUP:
    case WM_RBUTTONUP:
    case WM_XBUTTONUP:
    case WM_MOUSEMOVE:
	Tk_PointerEvent(hwnd, (short) LOWORD(lParam), (short) HIWORD(lParam));
	return 1;

    case WM_SYSKEYDOWN:
    case WM_KEYDOWN:
	if (wParam == VK_PACKET) {
	    /*
	     * This will trigger WM_CHAR event(s) with unicode data.
	     */
	    *resultPtr =
		PostMessageW(hwnd, message, HIWORD(lParam), LOWORD(lParam));
	    return 1;
	}
	/* else fall through */
    case WM_CLOSE:
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    case WM_DESTROYCLIPBOARD:
    case WM_UNICHAR:
    case WM_CHAR:
    case WM_SYSKEYUP:
    case WM_KEYUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
	GenerateXEvent(hwnd, message, wParam, lParam);
	return 1;
    case WM_MENUCHAR:
	GenerateXEvent(hwnd, message, wParam, lParam);

	/*
	 * MNC_CLOSE is the only one that looks right. This is a hack.
	 */

	*resultPtr = MAKELONG (0, MNC_CLOSE);
	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateXEvent --
 *
 *	This routine generates an X event from the corresponding Windows
 *	event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Queues one or more X events.
 *
 *----------------------------------------------------------------------
 */

static void
GenerateXEvent(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    XEvent event;
    TkWindow *winPtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if ((message == WM_MOUSEWHEEL) || (message == WM_MOUSEHWHEEL)) {
	union {LPARAM lParam; POINTS point;} root;
	POINT pos;
	root.lParam = lParam;

	/*
	 * Redirect mousewheel events to the window containing the cursor.
	 * That feels much less strange to users, and is how all the other
	 * platforms work.
	 */

	pos.x = root.point.x;
	pos.y = root.point.y;
	hwnd = WindowFromPoint(pos);
    }

    winPtr = (TkWindow *) Tk_HWNDToWindow(hwnd);
    if (!winPtr || winPtr->window == None) {
	return;
    }

    memset(&event, 0, sizeof(XEvent));
    event.xany.serial = winPtr->display->request++;
    event.xany.send_event = False;
    event.xany.display = winPtr->display;
    event.xany.window = winPtr->window;

    switch (message) {
    case WM_PAINT: {
	PAINTSTRUCT ps;

	event.type = Expose;
	BeginPaint(hwnd, &ps);
	event.xexpose.x = ps.rcPaint.left;
	event.xexpose.y = ps.rcPaint.top;
	event.xexpose.width = ps.rcPaint.right - ps.rcPaint.left;
	event.xexpose.height = ps.rcPaint.bottom - ps.rcPaint.top;
	EndPaint(hwnd, &ps);
	event.xexpose.count = 0;
	break;
    }

    case WM_CLOSE:
	event.type = ClientMessage;
	event.xclient.message_type =
		Tk_InternAtom((Tk_Window) winPtr, "WM_PROTOCOLS");
	event.xclient.format = 32;
	event.xclient.data.l[0] =
		Tk_InternAtom((Tk_Window) winPtr, "WM_DELETE_WINDOW");
	break;

    case WM_SETFOCUS:
    case WM_KILLFOCUS: {
	TkWindow *otherWinPtr = (TkWindow *) Tk_HWNDToWindow((HWND) wParam);

	/*
	 * Compare toplevel windows to avoid reporting focus changes within
	 * the same toplevel.
	 */

	while (!(winPtr->flags & TK_TOP_LEVEL)) {
	    winPtr = winPtr->parentPtr;
	    if (winPtr == NULL) {
		return;
	    }
	}
	while (otherWinPtr && !(otherWinPtr->flags & TK_TOP_LEVEL)) {
	    otherWinPtr = otherWinPtr->parentPtr;
	}

	/*
	 * Do a catch-all Tk_SetCaretPos here to make sure that the window
	 * receiving focus sets the caret at least once.
	 */

	if (message == WM_SETFOCUS) {
	    Tk_SetCaretPos((Tk_Window) winPtr, 0, 0, 0);
	}

	if (otherWinPtr == winPtr) {
	    return;
	}

	event.xany.window = winPtr->window;
	event.type = (message == WM_SETFOCUS) ? FocusIn : FocusOut;
	event.xfocus.mode = NotifyNormal;
	event.xfocus.detail = NotifyNonlinear;

	/*
	 * Destroy the caret if we own it. If we are moving to another Tk
	 * window, it will reclaim and reposition it with Tk_SetCaretPos.
	 */

	if (message == WM_KILLFOCUS) {
	    DestroyCaret();
	}
	break;
    }

    case WM_DESTROYCLIPBOARD:
	if (tsdPtr->updatingClipboard == TRUE) {
	    /*
	     * We want to avoid this event if we are the ones that caused this
	     * event.
	     */

	    return;
	}
	event.type = SelectionClear;
	event.xselectionclear.selection =
		Tk_InternAtom((Tk_Window)winPtr, "CLIPBOARD");
	event.xselectionclear.time = TkpGetMS();
	break;

    case WM_MOUSEWHEEL:
    case WM_MOUSEHWHEEL:
    case WM_CHAR:
    case WM_UNICHAR:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP: {
	unsigned int state = GetState(message, wParam, lParam);
	Time time = TkpGetMS();
	POINT clientPoint;
	union {DWORD msgpos; POINTS point;} root;	/* Note: POINT and POINTS are different */

	/*
	 * Compute the screen and window coordinates of the event.
	 */

	root.msgpos = GetMessagePos();
	clientPoint.x = root.point.x;
	clientPoint.y = root.point.y;
	ScreenToClient(hwnd, &clientPoint);

	/*
	 * Set up the common event fields.
	 */

	event.xbutton.root = RootWindow(winPtr->display, winPtr->screenNum);
	event.xbutton.subwindow = None;
	event.xbutton.x = clientPoint.x;
	event.xbutton.y = clientPoint.y;
	event.xbutton.x_root = root.point.x;
	event.xbutton.y_root = root.point.y;
	event.xbutton.state = state;
	event.xbutton.time = time;
	event.xbutton.same_screen = True;

	/*
	 * Now set up event specific fields.
	 */

	switch (message) {
	case WM_MOUSEWHEEL: {
	    /*
	     * Support for high resolution wheels (vertical).
	     */

	    DWORD wheelTick = GetTickCount();

	    if (wheelTick - tsdPtr->vWheelTickPrev < 1500) {
		tsdPtr->vWheelAcc += (short) HIWORD(wParam);
	    } else {
		tsdPtr->vWheelAcc = (short) HIWORD(wParam);
	    }
	    tsdPtr->vWheelTickPrev = wheelTick;
	    if (abs(tsdPtr->vWheelAcc) < WHEEL_DELTA) {
		return;
	    }

	    /*
	     * We have invented a new X event type to handle this event. It
	     * still uses the KeyPress struct. However, the keycode field has
	     * been overloaded to hold the zDelta of the wheel. Set nbytes to
	     * 0 to prevent conversion of the keycode to a keysym in
	     * TkpGetString. [Bug 1118340].
	     */

	    event.type = MouseWheelEvent;
	    event.xany.send_event = -1;
	    event.xkey.nbytes = 0;
	    event.xkey.keycode = tsdPtr->vWheelAcc / WHEEL_DELTA * WHEEL_DELTA;
	    tsdPtr->vWheelAcc = tsdPtr->vWheelAcc % WHEEL_DELTA;
	    break;
	}
	case WM_MOUSEHWHEEL: {
	    /*
	     * Support for high resolution wheels (horizontal).
	     */

	    DWORD wheelTick = GetTickCount();

	    if (wheelTick - tsdPtr->hWheelTickPrev < 1500) {
		tsdPtr->hWheelAcc -= (short) HIWORD(wParam);
	    } else {
		tsdPtr->hWheelAcc = -((short) HIWORD(wParam));
	    }
	    tsdPtr->hWheelTickPrev = wheelTick;
	    if (abs(tsdPtr->hWheelAcc) < WHEEL_DELTA) {
		return;
	    }

	    /*
	     * We have invented a new X event type to handle this event. It
	     * still uses the KeyPress struct. However, the keycode field has
	     * been overloaded to hold the zDelta of the wheel. Set nbytes to
	     * 0 to prevent conversion of the keycode to a keysym in
	     * TkpGetString. [Bug 1118340].
	     */

	    event.type = MouseWheelEvent;
	    event.xany.send_event = -1;
	    event.xkey.nbytes = 0;
	    event.xkey.state |= ShiftMask;
	    event.xkey.keycode = tsdPtr->hWheelAcc / WHEEL_DELTA * WHEEL_DELTA;
	    tsdPtr->hWheelAcc = tsdPtr->hWheelAcc % WHEEL_DELTA;
	    break;
	}
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
	    /*
	     * Check for translated characters in the event queue. Setting
	     * xany.send_event to -1 indicates to the Windows implementation
	     * of TkpGetString() that this event was generated by windows and
	     * that the Windows extension xkey.trans_chars is filled with the
	     * MBCS characters that came from the TranslateMessage call.
	     */

	    event.type = KeyPress;
	    event.xany.send_event = -1;
	    event.xkey.keycode = wParam;
	    GetTranslatedKey(&event.xkey, (message == WM_KEYDOWN) ? WM_CHAR :
	            WM_SYSCHAR);
	    break;

	case WM_SYSKEYUP:
	case WM_KEYUP:
	    /*
	     * We don't check for translated characters on keyup because Tk
	     * won't know what to do with them. Instead, we wait for the
	     * WM_CHAR messages which will follow.
	     */

	    event.type = KeyRelease;
	    event.xkey.keycode = wParam;
	    event.xkey.nbytes = 0;
	    break;

	case WM_CHAR:
	    /*
	     * Synthesize both a KeyPress and a KeyRelease. Strings generated
	     * by Input Method Editor are handled in the following manner:
	     * 1. A series of WM_KEYDOWN & WM_KEYUP messages that cause
	     *    GetTranslatedKey() to be called and return immediately
	     *    because the WM_KEYDOWNs have no associated WM_CHAR messages
	     *    -- the IME window is accumulating the characters and
	     *    translating them itself. In the "bind" command, you get an
	     *    event with a mystery keysym and %A == "" for each WM_KEYDOWN
	     *    that actually was meant for the IME.
	     * 2. A WM_KEYDOWN corresponding to the "confirm typing"
	     *    character. This causes GetTranslatedKey() to be called.
	     * 3. A WM_IME_NOTIFY message saying that the IME is done. A side
	     *	  effect of this message is that GetTranslatedKey() thinks
	     *	  this means that there are no WM_CHAR messages and returns
	     *	  immediately. In the "bind" command, you get an another event
	     *	  with a mystery keysym and %A == "".
	     * 4. A sequence of WM_CHAR messages that correspond to the
	     *	  characters in the IME window. A bunch of simulated
	     *	  KeyPress/KeyRelease events will be generated, one for each
	     *	  character. Adjacent WM_CHAR messages may actually specify
	     *	  the high and low bytes of a multi-byte character -- in that
	     *	  case the two WM_CHAR messages will be combined into one
	     *	  event. It is the event-consumer's responsibility to convert
	     *	  the string returned from XLookupString from system encoding
	     *	  to UTF-8.
	     * 5. And finally we get the WM_KEYUP for the "confirm typing"
	     *    character.
	     */

	    event.type = KeyPress;
	    event.xany.send_event = -1;
	    event.xkey.keycode = 0;
	    if ((int)wParam & 0xff00) {
		int ch1 = wParam & 0xffff;

		if ((ch1 & 0xfc00) == 0xd800) {
		    tsdPtr->surrogateBuffer = ch1;
		    return;
		}
		if ((ch1 & 0xfc00) == 0xdc00) {
		    ch1 = ((tsdPtr->surrogateBuffer & 0x3ff) << 10) |
			    (ch1 & 0x3ff) | 0x10000;
		    tsdPtr->surrogateBuffer = 0;
		}
		event.xany.send_event = -3;
		event.xkey.nbytes = 0;
		event.xkey.keycode = ch1;
	    } else {
		event.xkey.nbytes = 1;
		event.xkey.trans_chars[0] = (char) wParam;

		if (IsDBCSLeadByte((BYTE) wParam)) {
		    MSG msg;

		    if ((PeekMessageW(&msg, NULL, WM_CHAR, WM_CHAR,
		            PM_NOREMOVE) != 0)
			    && (msg.message == WM_CHAR)) {
			GetMessageW(&msg, NULL, WM_CHAR, WM_CHAR);
			event.xkey.nbytes = 2;
			event.xkey.trans_chars[1] = (char) msg.wParam;
		   }
		}
	    }
	    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	    event.type = KeyRelease;
	    break;

	case WM_UNICHAR: {
	    event.type = KeyPress;
	    event.xany.send_event = -3;
	    event.xkey.keycode = wParam;
	    event.xkey.nbytes = 0;
	    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	    event.type = KeyRelease;
	    break;
	}

	}
	break;
    }

    default:
	/*
	 * Don't know how to translate this event, so ignore it. (It probably
	 * should not have got here, but ignoring it should be harmless.)
	 */

	return;
    }

    /*
     * Post the translated event to the main Tk event queue.
     */

    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * GetState --
 *
 *	This function constructs a state mask for the mouse buttons and
 *	modifier keys as they were before the event occured.
 *
 * Results:
 *	Returns a composite value of all the modifier and button state flags
 *	that were set at the time the event occurred.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static unsigned int
GetState(
    UINT message,		/* Win32 message type */
    WPARAM wParam,		/* wParam of message, used if key message */
    LPARAM lParam)		/* lParam of message, used if key message */
{
    int mask;
    int prevState;		/* 1 if key was previously down */
    unsigned int state = TkWinGetModifierState();

    /*
     * If the event is a key press or release, we check for modifier keys so
     * we can report the state of the world before the event.
     */

    if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN
	    || message == WM_SYSKEYUP || message == WM_KEYUP) {
	mask = 0;
	prevState = HIWORD(lParam) & KF_REPEAT;
	switch(wParam) {
	case VK_SHIFT:
	    mask = ShiftMask;
	    break;
	case VK_CONTROL:
	    mask = ControlMask;
	    break;
	case VK_MENU:
	    mask = ALT_MASK;
	    break;
	case VK_CAPITAL:
	    if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN) {
		mask = LockMask;
		prevState = ((state & mask) ^ prevState) ? 0 : 1;
	    }
	    break;
	case VK_NUMLOCK:
	    if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN) {
		mask = Mod1Mask;
		prevState = ((state & mask) ^ prevState) ? 0 : 1;
	    }
	    break;
	case VK_SCROLL:
	    if (message == WM_SYSKEYDOWN || message == WM_KEYDOWN) {
		mask = Mod3Mask;
		prevState = ((state & mask) ^ prevState) ? 0 : 1;
	    }
	    break;
	}
	if (prevState) {
	    state |= mask;
	} else {
	    state &= ~mask;
	}
	if (HIWORD(lParam) & KF_EXTENDED) {
	    state |= EXTENDED_MASK;
	}
    }
    return state;
}

/*
 *----------------------------------------------------------------------
 *
 * GetTranslatedKey --
 *
 *	Retrieves WM_CHAR messages that are placed on the system queue by the
 *	TranslateMessage system call and places them in the given KeyPress
 *	event.
 *
 * Results:
 *	Sets the trans_chars and nbytes member of the key event.
 *
 * Side effects:
 *	Removes any WM_CHAR messages waiting on the top of the system event
 *	queue.
 *
 *----------------------------------------------------------------------
 */

static void
GetTranslatedKey(
    XKeyEvent *xkey,
    UINT type)
{
    MSG msg;

    xkey->nbytes = 0;

    while ((xkey->nbytes < XMaxTransChars)
	    && (PeekMessageA(&msg, NULL, type, type, PM_NOREMOVE) != 0)) {
	if (msg.message != type) {
	    break;
	}

	GetMessageA(&msg, NULL, type, type);

	/*
	 * If this is a normal character message, we may need to strip off the
	 * Alt modifier (e.g. Alt-digits). Note that we don't want to do this
	 * for system messages, because those were presumably generated as an
	 * Alt-char sequence (e.g. accelerator keys).
	 */

	if ((msg.message == WM_CHAR) && (msg.lParam & 0x20000000)) {
	    xkey->state = 0;
	}
	xkey->trans_chars[xkey->nbytes] = (char) msg.wParam;
	xkey->nbytes++;

	if (((unsigned short) msg.wParam) > ((unsigned short) 0xff)) {
	    /*
	     * Some "addon" input devices, such as the popular PenPower
	     * Chinese writing pad, generate 16 bit values in WM_CHAR messages
	     * (instead of passing them in two separate WM_CHAR messages
	     * containing two 8-bit values.
	     */

	    xkey->trans_chars[xkey->nbytes] = (char) (msg.wParam >> 8);
	    xkey->nbytes ++;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateInputLanguage --
 *
 *	Gets called when a WM_INPUTLANGCHANGE message is received by the Tk
 *	child window function. This message is sent by the Input Method Editor
 *	system when the user chooses a different input method. All subsequent
 *	WM_CHAR messages will contain characters in the new encoding. We
 *	record the new encoding so that TkpGetString() knows how to correctly
 *	translate the WM_CHAR into unicode.
 *
 * Results:
 *	Records the new encoding in keyInputEncoding.
 *
 * Side effects:
 *	Old value of keyInputEncoding is freed.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateInputLanguage(
    int charset)
{
    CHARSETINFO charsetInfo;
    Tcl_Encoding encoding;
    char codepage[4 + TCL_INTEGER_SPACE];

    if (keyInputCharset == charset) {
	return;
    }
    if (TranslateCharsetInfo(INT2PTR(charset), &charsetInfo,
	    TCI_SRCCHARSET) == 0) {
	/*
	 * Some mysterious failure.
	 */

	return;
    }

    sprintf(codepage, "cp%d", charsetInfo.ciACP);

    if ((encoding = Tcl_GetEncoding(NULL, codepage)) == NULL) {
	/*
	 * The encoding is not supported by Tcl.
	 */

	return;
    }

    if (keyInputEncoding != NULL) {
	Tcl_FreeEncoding(keyInputEncoding);
    }

    keyInputEncoding = encoding;
    keyInputCharset = charset;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetKeyInputEncoding --
 *
 *	Returns the current keyboard input encoding selected by the user (with
 *	WM_INPUTLANGCHANGE events).
 *
 * Results:
 *	The current keyboard input encoding.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Encoding
TkWinGetKeyInputEncoding(void)
{
    return keyInputEncoding;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetUnicodeEncoding --
 *
 *	Returns the cached unicode encoding.
 *
 * Results:
 *	The unicode encoding.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Encoding
TkWinGetUnicodeEncoding(void)
{
    if (unicodeEncoding == NULL) {
	unicodeEncoding = Tcl_GetEncoding(NULL, "utf-16");
	if (unicodeEncoding == NULL) {
	    unicodeEncoding = Tcl_GetEncoding(NULL, "unicode");
	}
    }
    return unicodeEncoding;
}

/*
 *----------------------------------------------------------------------
 *
 * HandleIMEComposition --
 *
 *	This function works around a deficiency in some versions of Windows
 *	2000 to make it possible to entry multi-lingual characters under all
 *	versions of Windows 2000.
 *
 *	When an Input Method Editor (IME) is ready to send input characters to
 *	an application, it sends a WM_IME_COMPOSITION message with the
 *	GCS_RESULTSTR. However, The DefWindowProcW() on English Windows 2000
 *	arbitrarily converts all non-Latin-1 characters in the composition to
 *	"?".
 *
 *	This function correctly processes the composition data and sends the
 *	UNICODE values of the composed characters to TK's event queue.
 *
 * Results:
 *	If this function has processed the composition data, returns 1.
 *	Otherwise returns 0.
 *
 * Side effects:
 *	Key events are put into the TK event queue.
 *
 *----------------------------------------------------------------------
 */

static int
HandleIMEComposition(
    HWND hwnd,			/* Window receiving the message. */
    LPARAM lParam)		/* Flags for the WM_IME_COMPOSITION message */
{
    HIMC hIMC;
    int n;
    int high = 0;

    if ((lParam & GCS_RESULTSTR) == 0) {
	/*
	 * Composition is not finished yet.
	 */

	return 0;
    }

    hIMC = ImmGetContext(hwnd);
    if (!hIMC) {
	return 0;
    }

    n = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);

    if (n > 0) {
	WCHAR *buff = (WCHAR *) ckalloc(n);
	TkWindow *winPtr;
	XEvent event;
	int i;

	n = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, buff, (unsigned) n) / 2;

	/*
	 * Set up the fields pertinent to key event.
	 *
	 * We set send_event to the special value of -3, so that TkpGetString
	 * in tkWinKey.c knows that keycode already contains a UNICODE
	 * char and there's no need to do encoding conversion.
	 *
	 * Note that the event *must* be zeroed out first; Tk plays cunning
	 * games with the overalls structure. [Bug 2992129]
	 */

	winPtr = (TkWindow *) Tk_HWNDToWindow(hwnd);

	memset(&event, 0, sizeof(XEvent));
	event.xkey.serial = winPtr->display->request++;
	event.xkey.send_event = -3;
	event.xkey.display = winPtr->display;
	event.xkey.window = winPtr->window;
	event.xkey.root = RootWindow(winPtr->display, winPtr->screenNum);
	event.xkey.subwindow = None;
	event.xkey.state = TkWinGetModifierState();
	event.xkey.time = TkpGetMS();
	event.xkey.same_screen = True;

	for (i=0; i<n; ) {
	    /*
	     * Simulate a pair of KeyPress and KeyRelease events for each
	     * UNICODE character in the composition.
	     */

	    event.xkey.keycode = buff[i++];

	    if ((event.xkey.keycode & 0xfc00) == 0xd800) {
		high = ((event.xkey.keycode & 0x3ff) << 10) + 0x10000;
		break;
	    } else if (high && (event.xkey.keycode & 0xfc00) == 0xdc00) {
		event.xkey.keycode &= 0x3ff;
		event.xkey.keycode += high;
		high = 0;
	    }
	    event.type = KeyPress;
	    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);

	    event.type = KeyRelease;
	    Tk_QueueWindowEvent(&event, TCL_QUEUE_TAIL);
	}

	ckfree(buff);
    }
    ImmReleaseContext(hwnd, hIMC);
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeXId --
 *
 *	This interface is not needed under Windows.
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
Tk_FreeXId(
    Display *display,
    XID xid)
{
    /* Do nothing */
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinResendEvent --
 *
 *	This function converts an X event into a Windows event and invokes the
 *	specified window function.
 *
 * Results:
 *	A standard Windows result.
 *
 * Side effects:
 *	Invokes the window function
 *
 *----------------------------------------------------------------------
 */

LRESULT
TkWinResendEvent(
    WNDPROC wndproc,
    HWND hwnd,
    XEvent *eventPtr)
{
    UINT msg;
    WPARAM wparam;
    LPARAM lparam;

    if (eventPtr->type != ButtonPress) {
	return 0;
    }

    switch (eventPtr->xbutton.button) {
    case Button1:
	msg = WM_LBUTTONDOWN;
	wparam = MK_LBUTTON;
	break;
    case Button2:
	msg = WM_MBUTTONDOWN;
	wparam = MK_MBUTTON;
	break;
    case Button3:
	msg = WM_RBUTTONDOWN;
	wparam = MK_RBUTTON;
	break;
    case Button4:
	msg = WM_XBUTTONDOWN;
	wparam = MAKEWPARAM(MK_XBUTTON1, XBUTTON1);
	break;
    case Button5:
	msg = WM_XBUTTONDOWN;
	wparam = MAKEWPARAM(MK_XBUTTON2, XBUTTON2);
	break;
    default:
	return 0;
    }

    if (eventPtr->xbutton.state & Button1Mask) {
	wparam |= MK_LBUTTON;
    }
    if (eventPtr->xbutton.state & Button2Mask) {
	wparam |= MK_MBUTTON;
    }
    if (eventPtr->xbutton.state & Button3Mask) {
	wparam |= MK_RBUTTON;
    }
    if (eventPtr->xbutton.state & Button4Mask) {
	wparam |= MK_XBUTTON1;
    }
    if (eventPtr->xbutton.state & Button5Mask) {
	wparam |= MK_XBUTTON2;
    }
    if (eventPtr->xbutton.state & ShiftMask) {
	wparam |= MK_SHIFT;
    }
    if (eventPtr->xbutton.state & ControlMask) {
	wparam |= MK_CONTROL;
    }
    lparam = MAKELPARAM((short) eventPtr->xbutton.x,
	    (short) eventPtr->xbutton.y);
    return CallWindowProcW(wndproc, hwnd, msg, wparam, lparam);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetMS --
 *
 *	Return a relative time in milliseconds. It doesn't matter when the
 *	epoch was.
 *
 * Results:
 *	Number of milliseconds.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

unsigned long
TkpGetMS(void)
{
    return GetTickCount();
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinUpdatingClipboard --
 *
 *
 * Results:
 *	Number of milliseconds.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkWinUpdatingClipboard(
    int mode)
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->updatingClipboard = mode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetCaretPos --
 *
 *	This enables correct movement of focus in the MS Magnifier, as well as
 *	allowing us to correctly position the IME Window. The following Win32
 *	APIs are used to work with MS caret:
 *
 *	CreateCaret	DestroyCaret	SetCaretPos	GetCaretPos
 *
 *	Only one instance of caret can be active at any time (e.g.
 *	DestroyCaret API does not take any argument such as handle). Since
 *	do-it-right approach requires to track the create/destroy caret status
 *	all the time in a global scope among windows (or widgets), we just
 *	implement this minimal setup to get the job done.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Sets the global Windows caret position.
 *
 *----------------------------------------------------------------------
 */

void
Tk_SetCaretPos(
    Tk_Window tkwin,
    int x, int y,
    int height)
{
    static HWND caretHWND = NULL;
    TkCaret *caretPtr = &(((TkWindow *) tkwin)->dispPtr->caret);
    Window win;

    /*
     * Prevent processing anything if the values haven't changed. Windows only
     * has one display, so we can do this with statics.
     */

    if ((caretPtr->winPtr == ((TkWindow *) tkwin))
	    && (caretPtr->x == x) && (caretPtr->y == y)) {
	return;
    }

    caretPtr->winPtr = ((TkWindow *) tkwin);
    caretPtr->x = x;
    caretPtr->y = y;
    caretPtr->height = height;

    /*
     * We adjust to the toplevel to get the coords right, as setting the IME
     * composition window is based on the toplevel hwnd, so ignore height.
     */

    while (!Tk_IsTopLevel(tkwin)) {
	x += Tk_X(tkwin);
	y += Tk_Y(tkwin);
	tkwin = Tk_Parent(tkwin);
	if (tkwin == NULL) {
	    return;
	}
    }

    win = Tk_WindowId(tkwin);
    if (win) {
	HIMC hIMC;
	HWND hwnd = Tk_GetHWND(win);

	if (hwnd != caretHWND) {
	    DestroyCaret();
	    if (CreateCaret(hwnd, NULL, 0, 0)) {
		caretHWND = hwnd;
	    }
	}

	if (!SetCaretPos(x, y) && CreateCaret(hwnd, NULL, 0, 0)) {
	    caretHWND = hwnd;
	    SetCaretPos(x, y);
	}

	/*
	 * The IME composition window should be updated whenever the caret
	 * position is changed because a clause of the composition string may
	 * be converted to the final characters and the other clauses still
	 * stay on the composition window. -- yamamoto
	 */

	hIMC = ImmGetContext(hwnd);
	if (hIMC) {
	    COMPOSITIONFORM cform;

	    cform.dwStyle = CFS_POINT;
	    cform.ptCurrentPos.x = x;
	    cform.ptCurrentPos.y = y;
	    ImmSetCompositionWindow(hIMC, &cform);
	    ImmReleaseContext(hwnd, hIMC);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetUserInactiveTime --
 *
 *	Return the number of milliseconds the user was inactive.
 *
 * Results:
 *	Milliseconds of user inactive time or -1 if the user32.dll doesn't
 *	have the symbol GetLastInputInfo or GetLastInputInfo returns an error.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

long
Tk_GetUserInactiveTime(
     Display *dpy)		/* Ignored on Windows */
{
    LASTINPUTINFO li;

    li.cbSize = sizeof(li);
    if (!(BOOL)GetLastInputInfo(&li)) {
	return -1;
    }

    /*
     * Last input info is in milliseconds, since restart time.
     */

    return (GetTickCount()-li.dwTime);
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ResetUserInactiveTime --
 *
 *	Reset the user inactivity timer
 *
 * Results:
 *	none
 *
 * Side effects:
 *	The user inactivity timer of the underlaying windowing system is reset
 *	to zero.
 *
 *----------------------------------------------------------------------
 */

void
Tk_ResetUserInactiveTime(
    Display *dpy)
{
    INPUT inp;

    inp.type = INPUT_MOUSE;
    inp.mi.dx = 0;
    inp.mi.dy = 0;
    inp.mi.mouseData = 0;
    inp.mi.dwFlags = MOUSEEVENTF_MOVE;
    inp.mi.time = 0;
    inp.mi.dwExtraInfo = (DWORD) 0;

    SendInput(1, &inp, sizeof(inp));
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
