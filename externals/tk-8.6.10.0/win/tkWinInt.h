/*
 * tkWinInt.h --
 *
 *	This file contains declarations that are shared among the
 *	Windows-specific parts of Tk, but aren't used by the rest of Tk.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TKWININT
#define _TKWININT

#ifndef _TKINT
#include "tkInt.h"
#endif

/*
 * Include platform specific public interfaces.
 */

#ifndef _TKWIN
#include "tkWin.h"
#endif

/*
 * Define constants missing from older Win32 SDK header files.
 */

#ifndef WS_EX_TOOLWINDOW
#define WS_EX_TOOLWINDOW	0x00000080L
#endif
#ifndef SPI_SETKEYBOARDCUES
#define SPI_SETKEYBOARDCUES 0x100B
#endif

/*
 * The TkWinDCState is used to save the state of a device context so that it
 * can be restored later.
 */

typedef struct TkWinDCState {
    HPALETTE palette;
    int bkmode;
} TkWinDCState;

/*
 * The TkWinDrawable is the internal implementation of an X Drawable (either a
 * Window or a Pixmap). The following constants define the valid Drawable
 * types.
 */

#define TWD_BITMAP	1
#define TWD_WINDOW	2
#define TWD_WINDC	3

typedef struct {
    int type;
    HWND handle;
    TkWindow *winPtr;
} TkWinWindow;

typedef struct {
    int type;
    HBITMAP handle;
    Colormap colormap;
    int depth;
} TkWinBitmap;

typedef struct {
    int type;
    HDC hdc;
}TkWinDC;

typedef union {
    int type;
    TkWinWindow window;
    TkWinBitmap bitmap;
    TkWinDC winDC;
} TkWinDrawable;

/*
 * The following macros are used to retrieve internal values from a Drawable.
 */

#define TkWinGetHWND(w)		(((TkWinDrawable *) w)->window.handle)
#define TkWinGetWinPtr(w)	(((TkWinDrawable *) w)->window.winPtr)
#define TkWinGetHBITMAP(w)	(((TkWinDrawable *) w)->bitmap.handle)
#define TkWinGetColormap(w)	(((TkWinDrawable *) w)->bitmap.colormap)
#define TkWinGetHDC(w)		(((TkWinDrawable *) w)->winDC.hdc)

/*
 * The following structure is used to encapsulate palette information.
 */

typedef struct {
    HPALETTE palette;		/* Palette handle used when drawing. */
    UINT size;			/* Number of entries in the palette. */
    int stale;			/* 1 if palette needs to be realized,
				 * otherwise 0. If the palette is stale, then
				 * an idle handler is scheduled to realize the
				 * palette. */
    Tcl_HashTable refCounts;	/* Hash table of palette entry reference
				 * counts indexed by pixel value. */
} TkWinColormap;

/*
 * The following macro retrieves the Win32 palette from a colormap.
 */

#define TkWinGetPalette(colormap) (((TkWinColormap *) colormap)->palette)

/*
 * The following macros define the class names for Tk Window types.
 */

#define TK_WIN_TOPLEVEL_CLASS_NAME L"TkTopLevel"
#define TK_WIN_CHILD_CLASS_NAME L"TkChild"

/*
 * The following variable is a translation table between X gc functions and
 * Win32 raster and BitBlt op modes.
 */

MODULE_SCOPE const int tkpWinRopModes[];
MODULE_SCOPE const int tkpWinBltModes[];

/*
 * The following defines are used with TkWinGetBorderPixels to get the extra 2
 * border colors from a Tk_3DBorder.
 */

#define TK_3D_LIGHT2 TK_3D_DARK_GC+1
#define TK_3D_DARK2 TK_3D_DARK_GC+2

/*
 * Internal functions used by more than one source file.
 */

#include "tkIntPlatDecls.h"

/*
 * Special proc needed as tsd accessor function between
 * tkWinX.c:GenerateXEvent and tkWinClipboard.c:UpdateClipboard
 */

MODULE_SCOPE void TkWinUpdatingClipboard(int mode);

/*
 * Used by tkWinDialog.c to associate the right icon with tk_messageBox
 */

MODULE_SCOPE HICON TkWinGetIcon(Tk_Window tkw, DWORD iconsize);

/*
 * Used by tkWinX.c on for certain system display change messages and cleanup
 * up containers
 */

MODULE_SCOPE void TkWinDisplayChanged(Display *display);
MODULE_SCOPE void TkWinCleanupContainerList(void);

/*
 * Used by tkWinWm.c for embedded menu handling. May become public.
 */

MODULE_SCOPE HWND Tk_GetMenuHWND(Tk_Window tkwin);
MODULE_SCOPE HWND Tk_GetEmbeddedMenuHWND(Tk_Window tkwin);

/*
 * The following allows us to cache these encoding for multiple functions.
 */


MODULE_SCOPE Tcl_Encoding	TkWinGetKeyInputEncoding(void);
MODULE_SCOPE Tcl_Encoding	TkWinGetUnicodeEncoding(void);
MODULE_SCOPE void		TkWinSetupSystemFonts(TkMainInfo *mainPtr);

/*
 * Values returned by TkWinGetPlatformTheme.
 */

#define TK_THEME_WIN_CLASSIC    1
#define TK_THEME_WIN_XP         2

/*
 * The following is implemented in tkWinWm and used by tkWinEmbed.c
 */

MODULE_SCOPE void		TkpWinToplevelWithDraw(TkWindow *winPtr);
MODULE_SCOPE void		TkpWinToplevelIconify(TkWindow *winPtr);
MODULE_SCOPE void		TkpWinToplevelDeiconify(TkWindow *winPtr);
MODULE_SCOPE long		TkpWinToplevelIsControlledByWm(TkWindow *winPtr);
MODULE_SCOPE long		TkpWinToplevelMove(TkWindow *winPtr, int x, int y);
MODULE_SCOPE long		TkpWinToplevelOverrideRedirect(TkWindow *winPtr,
			    int reqValue);
MODULE_SCOPE void		TkpWinToplevelDetachWindow(TkWindow *winPtr);
MODULE_SCOPE int		TkpWmGetState(TkWindow *winPtr);

/*
 * The following is implemented in tkWinPointer.c and also used in tkWinWindow.c
 */

MODULE_SCOPE void		TkSetCursorPos(int x, int y);

/*
 * Common routines used in Windows implementation
 */
MODULE_SCOPE Tcl_Obj *	        TkWin32ErrorObj(HRESULT hrError);


/*
 * The following functions are not present in old versions of Windows
 * API headers but are used in the Tk source to ensure 64bit
 * compatibility.
 */

#ifndef GetClassLongPtr
#   define GetClassLongPtrW	GetClassLongW
#   define SetClassLongPtrW	SetClassLongW
#endif /* !GetClassLongPtr */
#ifndef GCLP_HICON
#   define GCLP_HICON		GCL_HICON
#endif /* !GCLP_HICON */
#ifndef GCLP_HICONSM
#   define GCLP_HICONSM		(-34)
#endif /* !GCLP_HICONSM */

#ifndef GetWindowLongPtr
#   define GetWindowLongPtrW	GetWindowLongW
#   define SetWindowLongPtrW	SetWindowLongW
#endif /* !GetWindowLongPtr */
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC		GWL_WNDPROC
#define GWLP_HINSTANCE		GWL_HINSTANCE
#define GWLP_HWNDPARENT		GWL_HWNDPARENT
#define GWLP_USERDATA		GWL_USERDATA
#define GWLP_ID			GWL_ID
#endif /* !GWLP_WNDPROC */

#endif /* _TKWININT */
