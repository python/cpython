#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#endif

#include <tkWinInt.h>
#include "ttk/ttkTheme.h"

#if !defined(WM_THEMECHANGED)
#define WM_THEMECHANGED 0x031A
#endif

static LRESULT WINAPI WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

/*
 * RegisterSystemColors --
 *	Register all known Windows system colors (as per GetSysColor) as Tk
 *	named colors.
 */

typedef struct {
    const char *name;
    int index;
} SystemColorEntry;

static SystemColorEntry sysColors[] = {
	{ "System3dDarkShadow",		COLOR_3DDKSHADOW },
	{ "System3dLight",		COLOR_3DLIGHT },
	{ "SystemActiveBorder",		COLOR_ACTIVEBORDER },
	{ "SystemActiveCaption",	COLOR_ACTIVECAPTION },
	{ "SystemAppWorkspace",		COLOR_APPWORKSPACE },
	{ "SystemBackground",		COLOR_BACKGROUND },
	{ "SystemButtonFace",		COLOR_BTNFACE },
	{ "SystemButtonHighlight",	COLOR_BTNHIGHLIGHT },
	{ "SystemButtonShadow",		COLOR_BTNSHADOW },
	{ "SystemButtonText",		COLOR_BTNTEXT },
	{ "SystemCaptionText",		COLOR_CAPTIONTEXT },
	{ "SystemDisabledText",		COLOR_GRAYTEXT },
	{ "SystemGrayText",		COLOR_GRAYTEXT },
	{ "SystemHighlight",		COLOR_HIGHLIGHT },
	{ "SystemHighlightText",	COLOR_HIGHLIGHTTEXT },
	{ "SystemInactiveBorder",	COLOR_INACTIVEBORDER },
	{ "SystemInactiveCaption",	COLOR_INACTIVECAPTION },
	{ "SystemInactiveCaptionText",	COLOR_INACTIVECAPTIONTEXT },
	{ "SystemInfoBackground",	COLOR_INFOBK },
	{ "SystemInfoText",		COLOR_INFOTEXT },
	{ "SystemMenu",			COLOR_MENU },
	{ "SystemMenuText",		COLOR_MENUTEXT },
	{ "SystemScrollbar",		COLOR_SCROLLBAR },
	{ "SystemWindow",		COLOR_WINDOW },
	{ "SystemWindowFrame",		COLOR_WINDOWFRAME },
	{ "SystemWindowText",		COLOR_WINDOWTEXT },
	{ NULL, 0 }
};

static void RegisterSystemColors(Tcl_Interp *interp)
{
    Ttk_ResourceCache cache = Ttk_GetResourceCache(interp);
    SystemColorEntry *sysColor;

    for (sysColor = sysColors; sysColor->name; ++sysColor) {
	DWORD pixel = GetSysColor(sysColor->index);
	XColor colorSpec;
	colorSpec.red = GetRValue(pixel) * 257;
	colorSpec.green = GetGValue(pixel) * 257;
	colorSpec.blue = GetBValue(pixel) * 257;
	Ttk_RegisterNamedColor(cache, sysColor->name, &colorSpec);
    }
}

static HWND
CreateThemeMonitorWindow(HINSTANCE hinst, Tcl_Interp *interp)
{
    WNDCLASSEXW wc;
    HWND       hwnd = NULL;
    WCHAR      title[32] = L"TtkMonitorWindow";
    WCHAR      name[32] = L"TtkMonitorClass";

    wc.cbSize        = sizeof(WNDCLASSEXW);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hinst;
    wc.hIcon         = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wc.hIconSm       = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszMenuName  = name;
    wc.lpszClassName = name;

    if (RegisterClassExW(&wc)) {
	hwnd = CreateWindowW( name, title, WS_OVERLAPPEDWINDOW,
	    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
	    NULL, NULL, hinst, NULL );
	SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR) interp);
	ShowWindow(hwnd, SW_HIDE);
	UpdateWindow(hwnd);
    }
    return hwnd;
}

static void
DestroyThemeMonitorWindow(void *clientData)
{
    HWND hwnd = (HWND)clientData;
    DestroyWindow(hwnd);
}

static LRESULT WINAPI
WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    Tcl_Interp *interp = (Tcl_Interp *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    Ttk_Theme theme;

    switch (msg) {
    case WM_DESTROY:
	break;

    case WM_SYSCOLORCHANGE:
	RegisterSystemColors(interp);
	break;

    case WM_THEMECHANGED:
	/*
         * Reset the application theme.
         * On windows, it is possible to sign in as a second user, change
         * the theme to 'winnative' (by setting the ui to 'best performance'),
         * which is a machine-wide change, and then sign back on to the original user.
         * Ttk_UseTheme needs to be executed again in order to process the fallback
         * from vista/xpnative to winnative.
	 */

	theme = Ttk_GetCurrentTheme(interp);
	if (theme) {
	    Ttk_UseTheme(interp, theme);
	    /* @@@ What to do about errors here? */
	}
	break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/*
 * Windows-specific platform initialization:
 */

MODULE_SCOPE int TtkWinTheme_Init(Tcl_Interp *, HWND hwnd);
MODULE_SCOPE int TtkXPTheme_Init(Tcl_Interp *, HWND hwnd);

MODULE_SCOPE int Ttk_WinPlatformInit(Tcl_Interp *interp)
{
    HWND hwnd;

    hwnd = CreateThemeMonitorWindow(Tk_GetHINSTANCE(), interp);
    Ttk_RegisterCleanup(interp, (ClientData)hwnd, DestroyThemeMonitorWindow);

    TtkWinTheme_Init(interp, hwnd);
    TtkXPTheme_Init(interp, hwnd);

    return TCL_OK;
}
