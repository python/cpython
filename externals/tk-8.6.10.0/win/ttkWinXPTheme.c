/*
 * Tk theme engine which uses the Windows XP "Visual Styles" API
 * Adapted from Georgios Petasis' XP theme patch.
 *
 * Copyright (c) 2003 by Georgios Petasis, petasis@iit.demokritos.gr.
 * Copyright (c) 2003 by Joe English
 * Copyright (c) 2003 by Pat Thoyts
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * See also:
 *
 * <URL: http://msdn.microsoft.com/library/en-us/
 *  	shellcc/platform/commctls/userex/refentry.asp >
 */

#ifndef HAVE_UXTHEME_H
/* Stub for platforms that lack the XP theme API headers: */
#include <tkWinInt.h>
int TtkXPTheme_Init(Tcl_Interp *interp, HWND hwnd) { return TCL_OK; }
#else

#define WINVER 0x0501	/* Requires Windows XP APIs */

#include <windows.h>
#include <uxtheme.h>
#if defined(HAVE_VSSYM32_H) || _MSC_VER > 1500
#   include <vssym32.h>
#else
#   include <tmschema.h>
#endif

#include <tkWinInt.h>

#include "ttk/ttkTheme.h"

typedef HTHEME  (STDAPICALLTYPE OpenThemeDataProc)(HWND hwnd,
		 LPCWSTR pszClassList);
typedef HRESULT (STDAPICALLTYPE CloseThemeDataProc)(HTHEME hTheme);
typedef HRESULT (STDAPICALLTYPE DrawThemeBackgroundProc)(HTHEME hTheme,
                 HDC hdc, int iPartId, int iStateId, const RECT *pRect,
                 OPTIONAL const RECT *pClipRect);
typedef HRESULT	(STDAPICALLTYPE GetThemePartSizeProc)(HTHEME,HDC,
		 int iPartId, int iStateId,
		 RECT *prc, enum THEMESIZE eSize, SIZE *psz);
typedef int     (STDAPICALLTYPE GetThemeSysSizeProc)(HTHEME,int);
/* GetThemeTextExtent and DrawThemeText only used with BROKEN_TEXT_ELEMENT */
typedef HRESULT (STDAPICALLTYPE GetThemeTextExtentProc)(HTHEME hTheme, HDC hdc,
		 int iPartId, int iStateId, LPCWSTR pszText, int iCharCount,
		 DWORD dwTextFlags, const RECT *pBoundingRect, RECT *pExtent);
typedef HRESULT (STDAPICALLTYPE DrawThemeTextProc)(HTHEME hTheme, HDC hdc,
		 int iPartId, int iStateId, LPCWSTR pszText, int iCharCount,
		 DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect);
typedef BOOL    (STDAPICALLTYPE IsThemeActiveProc)(void);
typedef BOOL    (STDAPICALLTYPE IsAppThemedProc)(void);

typedef struct
{
    OpenThemeDataProc			*OpenThemeData;
    CloseThemeDataProc			*CloseThemeData;
    GetThemePartSizeProc		*GetThemePartSize;
    GetThemeSysSizeProc			*GetThemeSysSize;
    DrawThemeBackgroundProc		*DrawThemeBackground;
    DrawThemeTextProc		        *DrawThemeText;
    GetThemeTextExtentProc		*GetThemeTextExtent;
    IsThemeActiveProc			*IsThemeActive;
    IsAppThemedProc			*IsAppThemed;

    HWND                                stubWindow;
} XPThemeProcs;

typedef struct
{
    HINSTANCE hlibrary;
    XPThemeProcs *procs;
} XPThemeData;

/*
 *----------------------------------------------------------------------
 *
 * LoadXPThemeProcs --
 *	Initialize XP theming support.
 *
 *	XP theme support is included in UXTHEME.DLL
 *	We dynamically load this DLL at runtime instead of linking
 *	to it at build-time.
 *
 * Returns:
 *	A pointer to an XPThemeProcs table if successful, NULL otherwise.
 */

static XPThemeProcs *
LoadXPThemeProcs(HINSTANCE *phlib)
{
    /*
     * Load the library "uxtheme.dll", where the native widget
     * drawing routines are implemented.  This will only succeed
     * if we are running at least on Windows XP.
     */
    HINSTANCE handle;
    *phlib = handle = LoadLibraryW(L"uxtheme.dll");
    if (handle != 0)
    {
	/*
	 * We have successfully loaded the library. Proceed in storing the
	 * addresses of the functions we want to use.
	 */
	XPThemeProcs *procs = ckalloc(sizeof(XPThemeProcs));
#define LOADPROC(name) \
	(0 != (procs->name = (name ## Proc *)GetProcAddress(handle, #name) ))

	if (   LOADPROC(OpenThemeData)
	    && LOADPROC(CloseThemeData)
	    && LOADPROC(GetThemePartSize)
	    && LOADPROC(GetThemeSysSize)
	    && LOADPROC(DrawThemeBackground)
	    && LOADPROC(GetThemeTextExtent)
	    && LOADPROC(DrawThemeText)
	    && LOADPROC(IsThemeActive)
	    && LOADPROC(IsAppThemed)
	)
	{
	    return procs;
	}
#undef LOADPROC
	ckfree(procs);
    }
    return 0;
}

/*
 * XPThemeDeleteProc --
 *
 *      Release any theme allocated resources.
 */

static void
XPThemeDeleteProc(void *clientData)
{
    XPThemeData *themeData = clientData;
    FreeLibrary(themeData->hlibrary);
    ckfree(clientData);
}

static int
XPThemeEnabled(Ttk_Theme theme, void *clientData)
{
    XPThemeData *themeData = clientData;
    int active = themeData->procs->IsThemeActive();
    int themed = themeData->procs->IsAppThemed();
    return (active && themed);
}

/*
 * BoxToRect --
 * 	Helper routine.  Returns a RECT data structure.
 */
static RECT
BoxToRect(Ttk_Box b)
{
    RECT rc;
    rc.top = b.y;
    rc.left = b.x;
    rc.bottom = b.y + b.height;
    rc.right = b.x + b.width;
    return rc;
}

/*
 * Map Tk state bitmaps to XP style enumerated values.
 */
static Ttk_StateTable null_statemap[] = { {0,0,0} };

/*
 * Pushbuttons (Tk: "Button")
 */
static Ttk_StateTable pushbutton_statemap[] =
{
    { PBS_DISABLED, 	TTK_STATE_DISABLED, 0 },
    { PBS_PRESSED, 	TTK_STATE_PRESSED, 0 },
    { PBS_HOT,		TTK_STATE_ACTIVE, 0 },
    { PBS_DEFAULTED,	TTK_STATE_ALTERNATE, 0 },
    { PBS_NORMAL, 	0, 0 }
};

/*
 * Checkboxes (Tk: "Checkbutton")
 */
static Ttk_StateTable checkbox_statemap[] =
{
{CBS_MIXEDDISABLED, 	TTK_STATE_ALTERNATE|TTK_STATE_DISABLED, 0},
{CBS_MIXEDPRESSED, 	TTK_STATE_ALTERNATE|TTK_STATE_PRESSED, 0},
{CBS_MIXEDHOT,  	TTK_STATE_ALTERNATE|TTK_STATE_ACTIVE, 0},
{CBS_MIXEDNORMAL, 	TTK_STATE_ALTERNATE, 0},
{CBS_CHECKEDDISABLED,	TTK_STATE_SELECTED|TTK_STATE_DISABLED, 0},
{CBS_CHECKEDPRESSED,	TTK_STATE_SELECTED|TTK_STATE_PRESSED, 0},
{CBS_CHECKEDHOT,	TTK_STATE_SELECTED|TTK_STATE_ACTIVE, 0},
{CBS_CHECKEDNORMAL,	TTK_STATE_SELECTED, 0},
{CBS_UNCHECKEDDISABLED,	TTK_STATE_DISABLED, 0},
{CBS_UNCHECKEDPRESSED,	TTK_STATE_PRESSED, 0},
{CBS_UNCHECKEDHOT,	TTK_STATE_ACTIVE, 0},
{CBS_UNCHECKEDNORMAL,	0,0 }
};

/*
 * Radiobuttons:
 */
static Ttk_StateTable radiobutton_statemap[] =
{
{RBS_UNCHECKEDDISABLED,	TTK_STATE_ALTERNATE|TTK_STATE_DISABLED, 0},
{RBS_UNCHECKEDNORMAL,	TTK_STATE_ALTERNATE, 0},
{RBS_CHECKEDDISABLED,	TTK_STATE_SELECTED|TTK_STATE_DISABLED, 0},
{RBS_CHECKEDPRESSED,	TTK_STATE_SELECTED|TTK_STATE_PRESSED, 0},
{RBS_CHECKEDHOT,	TTK_STATE_SELECTED|TTK_STATE_ACTIVE, 0},
{RBS_CHECKEDNORMAL,	TTK_STATE_SELECTED, 0},
{RBS_UNCHECKEDDISABLED,	TTK_STATE_DISABLED, 0},
{RBS_UNCHECKEDPRESSED,	TTK_STATE_PRESSED, 0},
{RBS_UNCHECKEDHOT,	TTK_STATE_ACTIVE, 0},
{RBS_UNCHECKEDNORMAL,	0,0 }
};

/*
 * Groupboxes (tk: "frame")
 */
static Ttk_StateTable groupbox_statemap[] =
{
{GBS_DISABLED,	TTK_STATE_DISABLED, 0},
{GBS_NORMAL,	0,0 }
};

/*
 * Edit fields (tk: "entry")
 */
static Ttk_StateTable edittext_statemap[] =
{
    { ETS_DISABLED,	TTK_STATE_DISABLED, 0 },
    { ETS_READONLY,	TTK_STATE_READONLY, 0 },
    { ETS_FOCUSED,	TTK_STATE_FOCUS, 0 },
    { ETS_HOT,		TTK_STATE_ACTIVE, 0 },
    { ETS_NORMAL,	0, 0 }
/* NOT USED: ETS_ASSIST, ETS_SELECTED */
};

/*
 * Combobox text field statemap:
 * Same as edittext_statemap, but doesn't use ETS_READONLY
 * (fixes: #1032409)
 */
static Ttk_StateTable combotext_statemap[] =
{
    { ETS_DISABLED,	TTK_STATE_DISABLED, 0 },
    { ETS_FOCUSED,	TTK_STATE_FOCUS, 0 },
    { ETS_HOT,		TTK_STATE_ACTIVE, 0 },
    { ETS_NORMAL,	0, 0 }
};

/*
 * Combobox button: (CBP_DROPDOWNBUTTON)
 */
static Ttk_StateTable combobox_statemap[] = {
    { CBXS_DISABLED,	TTK_STATE_DISABLED, 0 },
    { CBXS_PRESSED, 	TTK_STATE_PRESSED, 0 },
    { CBXS_HOT, 	TTK_STATE_ACTIVE, 0 },
    { CBXS_HOT, 	TTK_STATE_HOVER, 0 },
    { CBXS_NORMAL, 	0, 0 }
};

/*
 * Toolbar buttons (TP_BUTTON):
 */
static Ttk_StateTable toolbutton_statemap[] =  {
    { TS_DISABLED, 	TTK_STATE_DISABLED, 0 },
    { TS_PRESSED,	TTK_STATE_PRESSED, 0 },
    { TS_HOTCHECKED,	TTK_STATE_SELECTED|TTK_STATE_ACTIVE, 0 },
    { TS_CHECKED, 	TTK_STATE_SELECTED, 0 },
    { TS_HOT,  		TTK_STATE_ACTIVE, 0 },
    { TS_NORMAL, 	0,0 }
};

/*
 * Scrollbars (Tk: "Scrollbar.thumb")
 */
static Ttk_StateTable scrollbar_statemap[] =
{
    { SCRBS_DISABLED, 	TTK_STATE_DISABLED, 0 },
    { SCRBS_PRESSED, 	TTK_STATE_PRESSED, 0 },
    { SCRBS_HOT,	TTK_STATE_ACTIVE, 0 },
    { SCRBS_NORMAL, 	0, 0 }
};

static Ttk_StateTable uparrow_statemap[] =
{
    { ABS_UPDISABLED,	TTK_STATE_DISABLED, 0 },
    { ABS_UPPRESSED, 	TTK_STATE_PRESSED, 0 },
    { ABS_UPHOT,	TTK_STATE_ACTIVE, 0 },
    { ABS_UPNORMAL, 	0, 0 }
};

static Ttk_StateTable downarrow_statemap[] =
{
    { ABS_DOWNDISABLED,	TTK_STATE_DISABLED, 0 },
    { ABS_DOWNPRESSED, 	TTK_STATE_PRESSED, 0 },
    { ABS_DOWNHOT,	TTK_STATE_ACTIVE, 0 },
    { ABS_DOWNNORMAL, 	0, 0 }
};

static Ttk_StateTable leftarrow_statemap[] =
{
    { ABS_LEFTDISABLED,	TTK_STATE_DISABLED, 0 },
    { ABS_LEFTPRESSED, 	TTK_STATE_PRESSED, 0 },
    { ABS_LEFTHOT,	TTK_STATE_ACTIVE, 0 },
    { ABS_LEFTNORMAL, 	0, 0 }
};

static Ttk_StateTable rightarrow_statemap[] =
{
    { ABS_RIGHTDISABLED,TTK_STATE_DISABLED, 0 },
    { ABS_RIGHTPRESSED, TTK_STATE_PRESSED, 0 },
    { ABS_RIGHTHOT,	TTK_STATE_ACTIVE, 0 },
    { ABS_RIGHTNORMAL, 	0, 0 }
};

static Ttk_StateTable spinbutton_statemap[] =
{
    { DNS_DISABLED,	TTK_STATE_DISABLED, 0 },
    { DNS_PRESSED,	TTK_STATE_PRESSED,  0 },
    { DNS_HOT,		TTK_STATE_ACTIVE,   0 },
    { DNS_NORMAL,	0,		    0 },
};

/*
 * Trackbar thumb: (Tk: "scale slider")
 */
static Ttk_StateTable scale_statemap[] =
{
    { TUS_DISABLED, 	TTK_STATE_DISABLED, 0 },
    { TUS_PRESSED, 	TTK_STATE_PRESSED, 0 },
    { TUS_FOCUSED, 	TTK_STATE_FOCUS, 0 },
    { TUS_HOT,		TTK_STATE_ACTIVE, 0 },
    { TUS_NORMAL, 	0, 0 }
};

static Ttk_StateTable tabitem_statemap[] =
{
    { TIS_DISABLED,     TTK_STATE_DISABLED, 0 },
    { TIS_SELECTED,     TTK_STATE_SELECTED, 0 },
    { TIS_HOT,          TTK_STATE_ACTIVE,   0 },
    { TIS_FOCUSED,      TTK_STATE_FOCUS,    0 },
    { TIS_NORMAL,       0,                  0 },
};


/*
 *----------------------------------------------------------------------
 * +++ Element data:
 *
 * The following structure is passed as the 'clientData' pointer
 * to most elements in this theme.  It contains data relevant
 * to a single XP Theme "part".
 *
 * <<NOTE-GetThemeMargins>>:
 *	In theory, we should be call GetThemeMargins(...TMT_CONTENTRECT...)
 *	to calculate the internal padding.  In practice, this routine
 *	only seems to work properly for BP_PUSHBUTTON.  So we hardcode
 *	the required padding at element registration time instead.
 *
 *	The PAD_MARGINS flag bit determines whether the padding
 *	should be added on the inside (0) or outside (1) of the element.
 *
 * <<NOTE-GetThemePartSize>>:
 *	This gives bogus metrics for some parts (in particular,
 *	BP_PUSHBUTTONS).  Set the IGNORE_THEMESIZE flag to skip this call.
 */

typedef struct 	/* XP element specifications */
{
    const char	*elementName;	/* Tk theme engine element name */
    Ttk_ElementSpec *elementSpec;
    				/* Element spec (usually GenericElementSpec) */
    LPCWSTR	className;	/* Windows window class name */
    int 	partId;		/* BP_PUSHBUTTON, BP_CHECKBUTTON, etc. */
    Ttk_StateTable *statemap;	/* Map Tk states to XP states */
    Ttk_Padding	padding;	/* See NOTE-GetThemeMargins */
    int  	flags;
#   define 	IGNORE_THEMESIZE 0x80000000 /* See NOTE-GetThemePartSize */
#   define 	PAD_MARGINS	 0x40000000 /* See NOTE-GetThemeMargins */
#   define 	HEAP_ELEMENT	 0x20000000 /* ElementInfo is on heap */
#   define 	HALF_HEIGHT	 0x10000000 /* Used by GenericSizedElements */
#   define 	HALF_WIDTH	 0x08000000 /* Used by GenericSizedElements */
} ElementInfo;

typedef struct
{
    /*
     * Static data, initialized when element is registered:
     */
    ElementInfo	*info;
    XPThemeProcs *procs;	/* Pointer to theme procedure table */

    /*
     * Dynamic data, allocated by InitElementData:
     */
    HTHEME	hTheme;
    HDC		hDC;
    HWND	hwnd;

    /* For TkWinDrawableReleaseDC: */
    Drawable	drawable;
    TkWinDCState dcState;
} ElementData;

static ElementData *
NewElementData(XPThemeProcs *procs, ElementInfo *info)
{
    ElementData *elementData = ckalloc(sizeof(ElementData));

    elementData->procs = procs;
    elementData->info = info;
    elementData->hTheme = elementData->hDC = 0;

    return elementData;
}

/*
 * Destroy elements. If the element was created by the element factory
 * then the info member is dynamically allocated. Otherwise it was
 * static data from the C object and only the ElementData needs freeing.
 */
static void DestroyElementData(void *clientData)
{
    ElementData *elementData = clientData;
    if (elementData->info->flags & HEAP_ELEMENT) {
	ckfree(elementData->info->statemap);
	ckfree(elementData->info->className);
	ckfree(elementData->info->elementName);
	ckfree(elementData->info);
    }
    ckfree(clientData);
}

/*
 * InitElementData --
 * 	Looks up theme handle.  If Drawable argument is non-NULL,
 * 	also initializes DC.
 *
 * Returns:
 * 	1 on success, 0 on error.
 * 	Caller must later call FreeElementData() so this element
 * 	can be reused.
 */

static int
InitElementData(ElementData *elementData, Tk_Window tkwin, Drawable d)
{
    Window win = Tk_WindowId(tkwin);

    if (win) {
	elementData->hwnd = Tk_GetHWND(win);
    } else  {
	elementData->hwnd = elementData->procs->stubWindow;
    }

    elementData->hTheme = elementData->procs->OpenThemeData(
	elementData->hwnd, elementData->info->className);

    if (!elementData->hTheme)
	return 0;

    elementData->drawable = d;
    if (d != 0) {
	elementData->hDC = TkWinGetDrawableDC(Tk_Display(tkwin), d,
	    &elementData->dcState);
    }

    return 1;
}

static void
FreeElementData(ElementData *elementData)
{
    elementData->procs->CloseThemeData(elementData->hTheme);
    if (elementData->drawable != 0) {
	TkWinReleaseDrawableDC(
	    elementData->drawable, elementData->hDC, &elementData->dcState);
    }
}

/*----------------------------------------------------------------------
 * +++ Generic element implementation.
 *
 * Used for elements which are handled entirely by the XP Theme API,
 * such as radiobutton and checkbutton indicators, scrollbar arrows, etc.
 */

static void GenericElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ElementData *elementData = clientData;
    HRESULT result;
    SIZE size;

    if (!InitElementData(elementData, tkwin, 0))
	return;

    if (!(elementData->info->flags & IGNORE_THEMESIZE)) {
	result = elementData->procs->GetThemePartSize(
	    elementData->hTheme,
	    elementData->hDC,
	    elementData->info->partId,
	    Ttk_StateTableLookup(elementData->info->statemap, 0),
	    NULL /*RECT *prc*/,
	    TS_TRUE,
	    &size);

	if (SUCCEEDED(result)) {
	    *widthPtr = size.cx;
	    *heightPtr = size.cy;
	}
    }

    /* See NOTE-GetThemeMargins
     */
    *paddingPtr = elementData->info->padding;
    if (elementData->info->flags & PAD_MARGINS) {
	*widthPtr += Ttk_PaddingWidth(elementData->info->padding);
	*heightPtr += Ttk_PaddingHeight(elementData->info->padding);
    }
}

static void GenericElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ElementData *elementData = clientData;
    RECT rc;

    if (!InitElementData(elementData, tkwin, d)) {
	return;
    }

    if (elementData->info->flags & PAD_MARGINS) {
    	b = Ttk_PadBox(b, elementData->info->padding);
    }
    rc = BoxToRect(b);

    elementData->procs->DrawThemeBackground(
	elementData->hTheme,
	elementData->hDC,
	elementData->info->partId,
	Ttk_StateTableLookup(elementData->info->statemap, state),
	&rc,
	NULL/*pContentRect*/);

    FreeElementData(elementData);
}

static Ttk_ElementSpec GenericElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    GenericElementSize,
    GenericElementDraw
};

/*----------------------------------------------------------------------
 * +++ Sized element implementation.
 *
 * Used for elements which are handled entirely by the XP Theme API,
 * but that require a fixed size adjustment.
 * Note that GetThemeSysSize calls through to GetSystemMetrics
 */

static void
GenericSizedElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ElementData *elementData = clientData;

    if (!InitElementData(elementData, tkwin, 0))
	return;

    GenericElementSize(clientData, elementRecord, tkwin,
	widthPtr, heightPtr, paddingPtr);

    *widthPtr = elementData->procs->GetThemeSysSize(NULL,
	(elementData->info->flags >> 8) & 0xff);
    *heightPtr = elementData->procs->GetThemeSysSize(NULL,
	elementData->info->flags & 0xff);
    if (elementData->info->flags & HALF_HEIGHT)
	*heightPtr /= 2;
    if (elementData->info->flags & HALF_WIDTH)
	*widthPtr /= 2;
}

static Ttk_ElementSpec GenericSizedElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    GenericSizedElementSize,
    GenericElementDraw
};

/*----------------------------------------------------------------------
 * +++ Spinbox arrow element.
 *     These are half-height scrollbar buttons.
 */

static void
SpinboxArrowElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ElementData *elementData = clientData;

    if (!InitElementData(elementData, tkwin, 0))
	return;

    GenericSizedElementSize(clientData, elementRecord, tkwin,
	widthPtr, heightPtr, paddingPtr);

    /* force the arrow button height to half size */
    *heightPtr /= 2;
}

static Ttk_ElementSpec SpinboxArrowElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    SpinboxArrowElementSize,
    GenericElementDraw
};

/*----------------------------------------------------------------------
 * +++ Scrollbar thumb element.
 *     Same as a GenericElement, but don't draw in the disabled state.
 */

static void ThumbElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ElementData *elementData = clientData;
    unsigned stateId = Ttk_StateTableLookup(elementData->info->statemap, state);
    RECT rc = BoxToRect(b);

    /*
     * Don't draw the thumb if we are disabled.
     */
    if (state & TTK_STATE_DISABLED)
	return;

    if (!InitElementData(elementData, tkwin, d))
	return;

    elementData->procs->DrawThemeBackground(elementData->hTheme,
	elementData->hDC, elementData->info->partId, stateId,
	&rc, NULL);

    FreeElementData(elementData);
}

static Ttk_ElementSpec ThumbElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    GenericElementSize,
    ThumbElementDraw
};

/*----------------------------------------------------------------------
 * +++ Progress bar element.
 *	Increases the requested length of PP_CHUNK and PP_CHUNKVERT parts
 *	so that indeterminate progress bars show 3 bars instead of 1.
 */

static void PbarElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ElementData *elementData = clientData;
    int nBars = 3;

    GenericElementSize(clientData, elementRecord, tkwin,
    	widthPtr, heightPtr, paddingPtr);

    if (elementData->info->partId == PP_CHUNK) {
    	*widthPtr *= nBars;
    } else if (elementData->info->partId == PP_CHUNKVERT) {
    	*heightPtr *= nBars;
    }
}

static Ttk_ElementSpec PbarElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    PbarElementSize,
    GenericElementDraw
};

/*----------------------------------------------------------------------
 * +++  Notebook tab element.
 *	Same as generic element, with additional logic to select
 *	proper iPartID for the leftmost tab.
 *
 *	Notes: TABP_TABITEMRIGHTEDGE (or TABP_TOPTABITEMRIGHTEDGE,
 * 	which appears to be identical) should be used if the
 *	tab is exactly at the right edge of the notebook, but
 *	not if it's simply the rightmost tab.  This information
 * 	is not available.
 *
 *	The TIS_* and TILES_* definitions are identical, so
 * 	we can use the same statemap no matter what the partId.
 */
static void TabElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ElementData *elementData = clientData;
    int partId = elementData->info->partId;
    RECT rc = BoxToRect(b);

    if (!InitElementData(elementData, tkwin, d))
	return;
    if (state & TTK_STATE_USER1)
	partId = TABP_TABITEMLEFTEDGE;
    elementData->procs->DrawThemeBackground(
	elementData->hTheme, elementData->hDC, partId,
	Ttk_StateTableLookup(elementData->info->statemap, state), &rc, NULL);
    FreeElementData(elementData);
}

static Ttk_ElementSpec TabElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    GenericElementSize,
    TabElementDraw
};

/*----------------------------------------------------------------------
 * +++  Tree indicator element.
 *
 *	Generic element, but don't display at all if TTK_STATE_LEAF (=USER2) set
 */

#define TTK_STATE_OPEN TTK_STATE_USER1
#define TTK_STATE_LEAF TTK_STATE_USER2

static Ttk_StateTable header_statemap[] =
{
    { HIS_PRESSED, 	TTK_STATE_PRESSED, 0 },
    { HIS_HOT,  	TTK_STATE_ACTIVE, 0 },
    { HIS_NORMAL, 	0,0 },
};

static Ttk_StateTable treeview_statemap[] =
{
    { TREIS_DISABLED, 	TTK_STATE_DISABLED, 0 },
    { TREIS_SELECTED,	TTK_STATE_SELECTED, 0},
    { TREIS_HOT, 	TTK_STATE_ACTIVE, 0 },
    { TREIS_NORMAL, 	0,0 },
};

static Ttk_StateTable tvpglyph_statemap[] =
{
    { GLPS_OPENED, 	TTK_STATE_OPEN, 0 },
    { GLPS_CLOSED, 	0,0 },
};

static void TreeIndicatorElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    if (!(state & TTK_STATE_LEAF)) {
        GenericElementDraw(clientData,elementRecord,tkwin,d,b,state);
    }
}

static Ttk_ElementSpec TreeIndicatorElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    GenericElementSize,
    TreeIndicatorElementDraw
};

#if BROKEN_TEXT_ELEMENT

/*
 *----------------------------------------------------------------------
 * Text element (does not work yet).
 *
 * According to "Using Windows XP Visual Styles",  we need to select
 * a font into the DC before calling DrawThemeText().
 * There's just no easy way to get an HFONT out of a Tk_Font.
 * Maybe GetThemeFont() would work?
 *
 */

typedef struct
{
    Tcl_Obj *textObj;
    Tcl_Obj *fontObj;
} TextElement;

static Ttk_ElementOptionSpec TextElementOptions[] =
{
    { "-text", TK_OPTION_STRING,
	Tk_Offset(TextElement,textObj), "" },
    { "-font", TK_OPTION_FONT,
	Tk_Offset(TextElement,fontObj), DEFAULT_FONT },
    { NULL }
};

static void TextElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    TextElement *element = elementRecord;
    ElementData *elementData = clientData;
    RECT rc = {0, 0};
    HRESULT hr = S_OK;
    const char *src;
    int len;
    Tcl_DString ds;

    if (!InitElementData(elementData, tkwin, 0))
	return;

    src = Tcl_GetStringFromObj(element->textObj, &len);
    Tcl_WinUtfToTChar(src, len, &ds);
    hr = elementData->procs->GetThemeTextExtent(
	    elementData->hTheme,
	    elementData->hDC,
	    elementData->info->partId,
	    Ttk_StateTableLookup(elementData->info->statemap, 0),
	    (WCHAR *) Tcl_DStringValue(&ds),
	    -1,
	    DT_LEFT,// | DT_BOTTOM | DT_NOPREFIX,
	    NULL,
	    &rc);

    if (SUCCEEDED(hr)) {
	*widthPtr = rc.right - rc.left;
	*heightPtr = rc.bottom - rc.top;
    }
    if (*widthPtr < 80) *widthPtr = 80;
    if (*heightPtr < 20) *heightPtr = 20;

    Tcl_DStringFree(&ds);
    FreeElementData(elementData);
}

static void TextElementDraw(
    ClientData clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    TextElement *element = elementRecord;
    ElementData *elementData = clientData;
    RECT rc = BoxToRect(b);
    HRESULT hr = S_OK;
    const char *src;
    int len;
    Tcl_DString ds;

    if (!InitElementData(elementData, tkwin, d))
	return;

    src = Tcl_GetStringFromObj(element->textObj, &len);
    Tcl_WinUtfToTChar(src, len, &ds);
    hr = elementData->procs->DrawThemeText(
	    elementData->hTheme,
	    elementData->hDC,
	    elementData->info->partId,
	    Ttk_StateTableLookup(elementData->info->statemap, state),
	    (WCHAR *) Tcl_DStringValue(&ds),
	    -1,
	    DT_LEFT,// | DT_BOTTOM | DT_NOPREFIX,
	    (state & TTK_STATE_DISABLED) ? DTT_GRAYED : 0,
	    &rc);

    Tcl_DStringFree(&ds);
    FreeElementData(elementData);
}

static Ttk_ElementSpec TextElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(TextElement),
    TextElementOptions,
    TextElementSize,
    TextElementDraw
};

#endif	/* BROKEN_TEXT_ELEMENT */

/*----------------------------------------------------------------------
 * +++ Widget layouts:
 */

TTK_BEGIN_LAYOUT_TABLE(LayoutTable)

TTK_LAYOUT("TButton",
    TTK_GROUP("Button.button", TTK_FILL_BOTH,
	TTK_GROUP("Button.focus", TTK_FILL_BOTH,
	    TTK_GROUP("Button.padding", TTK_FILL_BOTH,
		TTK_NODE("Button.label", TTK_FILL_BOTH)))))

TTK_LAYOUT("TMenubutton",
    TTK_NODE("Menubutton.dropdown", TTK_PACK_RIGHT|TTK_FILL_Y)
    TTK_GROUP("Menubutton.button", TTK_PACK_RIGHT|TTK_EXPAND|TTK_FILL_BOTH,
	    TTK_GROUP("Menubutton.padding", TTK_PACK_LEFT|TTK_EXPAND|TTK_FILL_X,
	        TTK_NODE("Menubutton.label", 0))))

TTK_LAYOUT("Horizontal.TScrollbar",
    TTK_GROUP("Horizontal.Scrollbar.trough", TTK_FILL_X,
	TTK_NODE("Horizontal.Scrollbar.leftarrow", TTK_PACK_LEFT)
	TTK_NODE("Horizontal.Scrollbar.rightarrow", TTK_PACK_RIGHT)
	TTK_GROUP("Horizontal.Scrollbar.thumb", TTK_FILL_BOTH|TTK_UNIT,
	    TTK_NODE("Horizontal.Scrollbar.grip", 0))))

TTK_LAYOUT("Vertical.TScrollbar",
    TTK_GROUP("Vertical.Scrollbar.trough", TTK_FILL_Y,
	TTK_NODE("Vertical.Scrollbar.uparrow", TTK_PACK_TOP)
	TTK_NODE("Vertical.Scrollbar.downarrow", TTK_PACK_BOTTOM)
	TTK_GROUP("Vertical.Scrollbar.thumb", TTK_FILL_BOTH|TTK_UNIT,
	    TTK_NODE("Vertical.Scrollbar.grip", 0))))

TTK_LAYOUT("Horizontal.TScale",
    TTK_GROUP("Scale.focus", TTK_EXPAND|TTK_FILL_BOTH,
	TTK_GROUP("Horizontal.Scale.trough", TTK_EXPAND|TTK_FILL_BOTH,
	    TTK_NODE("Horizontal.Scale.track", TTK_FILL_X)
	    TTK_NODE("Horizontal.Scale.slider", TTK_PACK_LEFT) )))

TTK_LAYOUT("Vertical.TScale",
    TTK_GROUP("Scale.focus", TTK_EXPAND|TTK_FILL_BOTH,
	TTK_GROUP("Vertical.Scale.trough", TTK_EXPAND|TTK_FILL_BOTH,
	    TTK_NODE("Vertical.Scale.track", TTK_FILL_Y)
	    TTK_NODE("Vertical.Scale.slider", TTK_PACK_TOP) )))

TTK_END_LAYOUT_TABLE

/*----------------------------------------------------------------------
 * +++ XP element info table:
 */

#define PAD(l,t,r,b) {l,t,r,b}
#define NOPAD {0,0,0,0}

/* name spec className partId statemap padding flags */

static ElementInfo ElementInfoTable[] = {
    { "Checkbutton.indicator", &GenericElementSpec, L"BUTTON",
    	BP_CHECKBOX, checkbox_statemap, PAD(0, 0, 4, 0), PAD_MARGINS },
    { "Radiobutton.indicator", &GenericElementSpec, L"BUTTON",
    	BP_RADIOBUTTON, radiobutton_statemap, PAD(0, 0, 4, 0), PAD_MARGINS },
    { "Button.button", &GenericElementSpec, L"BUTTON",
    	BP_PUSHBUTTON, pushbutton_statemap, PAD(3, 3, 3, 3), IGNORE_THEMESIZE },
    { "Labelframe.border", &GenericElementSpec, L"BUTTON",
    	BP_GROUPBOX, groupbox_statemap, PAD(2, 2, 2, 2), 0 },
    { "Entry.field", &GenericElementSpec, L"EDIT", EP_EDITTEXT,
    	edittext_statemap, PAD(1, 1, 1, 1), 0 },
    { "Combobox.field", &GenericElementSpec, L"EDIT",
	EP_EDITTEXT, combotext_statemap, PAD(1, 1, 1, 1), 0 },
    { "Combobox.downarrow", &GenericSizedElementSpec, L"COMBOBOX",
	CP_DROPDOWNBUTTON, combobox_statemap, NOPAD,
	(SM_CXVSCROLL << 8) | SM_CYVSCROLL },
    { "Vertical.Scrollbar.trough", &GenericElementSpec, L"SCROLLBAR",
    	SBP_UPPERTRACKVERT, scrollbar_statemap, NOPAD, 0 },
    { "Vertical.Scrollbar.thumb", &ThumbElementSpec, L"SCROLLBAR",
    	SBP_THUMBBTNVERT, scrollbar_statemap, NOPAD, 0 },
    { "Vertical.Scrollbar.grip", &GenericElementSpec, L"SCROLLBAR",
    	SBP_GRIPPERVERT, scrollbar_statemap, NOPAD, 0 },
    { "Horizontal.Scrollbar.trough", &GenericElementSpec, L"SCROLLBAR",
    	SBP_UPPERTRACKHORZ, scrollbar_statemap, NOPAD, 0 },
    { "Horizontal.Scrollbar.thumb", &ThumbElementSpec, L"SCROLLBAR",
   	SBP_THUMBBTNHORZ, scrollbar_statemap, NOPAD, 0 },
    { "Horizontal.Scrollbar.grip", &GenericElementSpec, L"SCROLLBAR",
    	SBP_GRIPPERHORZ, scrollbar_statemap, NOPAD, 0 },
    { "Scrollbar.uparrow", &GenericSizedElementSpec, L"SCROLLBAR",
    	SBP_ARROWBTN, uparrow_statemap, NOPAD,
	(SM_CXVSCROLL << 8) | SM_CYVSCROLL },
    { "Scrollbar.downarrow", &GenericSizedElementSpec, L"SCROLLBAR",
    	SBP_ARROWBTN, downarrow_statemap, NOPAD,
	(SM_CXVSCROLL << 8) | SM_CYVSCROLL },
    { "Scrollbar.leftarrow", &GenericSizedElementSpec, L"SCROLLBAR",
    	SBP_ARROWBTN, leftarrow_statemap, NOPAD,
	(SM_CXHSCROLL << 8) | SM_CYHSCROLL },
    { "Scrollbar.rightarrow", &GenericSizedElementSpec, L"SCROLLBAR",
    	SBP_ARROWBTN, rightarrow_statemap, NOPAD,
	(SM_CXHSCROLL << 8) | SM_CYHSCROLL },
    { "Horizontal.Scale.slider", &GenericElementSpec, L"TRACKBAR",
    	TKP_THUMB, scale_statemap, NOPAD, 0 },
    { "Vertical.Scale.slider", &GenericElementSpec, L"TRACKBAR",
    	TKP_THUMBVERT, scale_statemap, NOPAD, 0 },
    { "Horizontal.Scale.track", &GenericElementSpec, L"TRACKBAR",
    	TKP_TRACK, scale_statemap, NOPAD, 0 },
    { "Vertical.Scale.track", &GenericElementSpec, L"TRACKBAR",
    	TKP_TRACKVERT, scale_statemap, NOPAD, 0 },
    /* ttk::progressbar elements */
    { "Horizontal.Progressbar.pbar", &PbarElementSpec, L"PROGRESS",
    	PP_CHUNK, null_statemap, NOPAD, 0 },
    { "Vertical.Progressbar.pbar", &PbarElementSpec, L"PROGRESS",
    	PP_CHUNKVERT, null_statemap, NOPAD, 0 },
    { "Horizontal.Progressbar.trough", &GenericElementSpec, L"PROGRESS",
    	PP_BAR, null_statemap, PAD(3,3,3,3), IGNORE_THEMESIZE },
    { "Vertical.Progressbar.trough", &GenericElementSpec, L"PROGRESS",
    	PP_BARVERT, null_statemap, PAD(3,3,3,3), IGNORE_THEMESIZE },
    /* ttk::notebook */
    { "tab", &TabElementSpec, L"TAB",
    	TABP_TABITEM, tabitem_statemap, PAD(3,3,3,0), 0 },
    { "client", &GenericElementSpec, L"TAB",
    	TABP_PANE, null_statemap, PAD(1,1,3,3), 0 },
    { "NotebookPane.background", &GenericElementSpec, L"TAB",
    	TABP_BODY, null_statemap, NOPAD, 0 },
    { "Toolbutton.border", &GenericElementSpec, L"TOOLBAR",
    	TP_BUTTON, toolbutton_statemap, NOPAD,0 },
    { "Menubutton.button", &GenericElementSpec, L"TOOLBAR",
    	TP_SPLITBUTTON,toolbutton_statemap, NOPAD,0 },
    { "Menubutton.dropdown", &GenericElementSpec, L"TOOLBAR",
    	TP_SPLITBUTTONDROPDOWN,toolbutton_statemap, NOPAD,0 },
    { "Treeview.field", &GenericElementSpec, L"TREEVIEW",
	TVP_TREEITEM, treeview_statemap, PAD(1, 1, 1, 1), 0 },
    { "Treeitem.indicator", &TreeIndicatorElementSpec, L"TREEVIEW",
    	TVP_GLYPH, tvpglyph_statemap, PAD(1,1,6,0), PAD_MARGINS },
    { "Treeheading.border", &GenericElementSpec, L"HEADER",
    	HP_HEADERITEM, header_statemap, PAD(4,0,4,0),0 },
    { "sizegrip", &GenericElementSpec, L"STATUS",
    	SP_GRIPPER, null_statemap, NOPAD,0 },
    { "Spinbox.field", &GenericElementSpec, L"EDIT",
	EP_EDITTEXT, edittext_statemap, PAD(1, 1, 1, 1), 0 },
    { "Spinbox.uparrow", &SpinboxArrowElementSpec, L"SPIN",
	SPNP_UP, spinbutton_statemap, NOPAD,
	PAD_MARGINS | ((SM_CXVSCROLL << 8) | SM_CYVSCROLL) },
    { "Spinbox.downarrow", &SpinboxArrowElementSpec, L"SPIN",
	SPNP_DOWN, spinbutton_statemap, NOPAD,
	PAD_MARGINS | ((SM_CXVSCROLL << 8) | SM_CYVSCROLL) },

#if BROKEN_TEXT_ELEMENT
    { "Labelframe.text", &TextElementSpec, L"BUTTON",
    	BP_GROUPBOX, groupbox_statemap, NOPAD,0 },
#endif

    { 0,0,0,0,0,NOPAD,0 }
};
#undef PAD


static int
GetSysFlagFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *resultPtr)
{
    static const char *names[] = {
	"SM_CXBORDER", "SM_CYBORDER", "SM_CXVSCROLL", "SM_CYVSCROLL",
	"SM_CXHSCROLL", "SM_CYHSCROLL", "SM_CXMENUCHECK", "SM_CYMENUCHECK",
	"SM_CXMENUSIZE", "SM_CYMENUSIZE", "SM_CXSIZE", "SM_CYSIZE", "SM_CXSMSIZE",
	"SM_CYSMSIZE", NULL
    };
    int flags[] = {
	SM_CXBORDER, SM_CYBORDER, SM_CXVSCROLL, SM_CYVSCROLL,
	SM_CXHSCROLL, SM_CYHSCROLL, SM_CXMENUCHECK, SM_CYMENUCHECK,
	SM_CXMENUSIZE, SM_CYMENUSIZE, SM_CXSIZE, SM_CYSIZE, SM_CXSMSIZE,
	SM_CYSMSIZE
    };

    Tcl_Obj **objv;
    int i, objc;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK)
	return TCL_ERROR;
    if (objc != 2) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj("wrong # args", -1));
	Tcl_SetErrorCode(interp, "TCL", "WRONGARGS", NULL);
	return TCL_ERROR;
    }
    for (i = 0; i < objc; ++i) {
	int option;
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], names,
		sizeof(char *), "system constant", 0, &option) != TCL_OK)
	    return TCL_ERROR;
	*resultPtr |= (flags[option] << (8 * (1 - i)));
    }
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Windows Visual Styles API Element Factory
 *
 * The Vista release has shown that the Windows Visual Styles can be
 * extended with additional elements. This element factory can permit
 * the programmer to create elements for use with script-defined layouts
 *
 * eg: to create the small close button:
 * style element create smallclose vsapi \
 *    WINDOW 19 {disabled 4 pressed 3 active 2 {} 1}
 */

static int
Ttk_CreateVsapiElement(
    Tcl_Interp *interp,
    void *clientData,
    Ttk_Theme theme,
    const char *elementName,
    int objc,
    Tcl_Obj *const objv[])
{
    XPThemeData *themeData = clientData;
    ElementInfo *elementPtr = NULL;
    ClientData elementData;
    LPCWSTR className;
    int partId = 0;
    Ttk_StateTable *stateTable;
    Ttk_Padding pad = {0, 0, 0, 0};
    int flags = 0;
    int length = 0;
    char *name;
    LPWSTR wname;
    Ttk_ElementSpec *elementSpec = &GenericElementSpec;
    Tcl_DString classBuf;

    static const char *optionStrings[] =
	{ "-padding","-width","-height","-margins", "-syssize",
	  "-halfheight", "-halfwidth", NULL };
    enum { O_PADDING, O_WIDTH, O_HEIGHT, O_MARGINS, O_SYSSIZE,
	   O_HALFHEIGHT, O_HALFWIDTH };

    if (objc < 2) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
	    "missing required arguments 'class' and/or 'partId'", -1));
	Tcl_SetErrorCode(interp, "TTK", "VSAPI", "REQUIRED", NULL);
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[1], &partId) != TCL_OK) {
	return TCL_ERROR;
    }
    name = Tcl_GetStringFromObj(objv[0], &length);
    className = (LPCWSTR) Tcl_WinUtfToTChar(name, length, &classBuf);

    /* flags or padding */
    if (objc > 3) {
	int i = 3, option = 0;
	for (i = 3; i < objc; i += 2) {
	    int tmp = 0;
	    if (i == objc -1) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"Missing value for \"%s\".",
			Tcl_GetString(objv[i])));
		Tcl_SetErrorCode(interp, "TTK", "VSAPI", "MISSING", NULL);
		goto retErr;
	    }
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i], optionStrings,
		    sizeof(char *), "option", 0, &option) != TCL_OK)
		goto retErr;
	    switch (option) {
	    case O_PADDING:
		if (Ttk_GetBorderFromObj(interp, objv[i+1], &pad) != TCL_OK) {
		    goto retErr;
		}
		break;
	    case O_MARGINS:
		if (Ttk_GetBorderFromObj(interp, objv[i+1], &pad) != TCL_OK) {
		    goto retErr;
		}
		flags |= PAD_MARGINS;
		break;
	    case O_WIDTH:
		if (Tcl_GetIntFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    goto retErr;
		}
		pad.left = pad.right = tmp;
		flags |= IGNORE_THEMESIZE;
		break;
	    case O_HEIGHT:
		if (Tcl_GetIntFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    goto retErr;
		}
		pad.top = pad.bottom = tmp;
		flags |= IGNORE_THEMESIZE;
		break;
	    case O_SYSSIZE:
		if (GetSysFlagFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    goto retErr;
		}
		elementSpec = &GenericSizedElementSpec;
		flags |= (tmp & 0xFFFF);
		break;
	    case O_HALFHEIGHT:
		if (Tcl_GetBooleanFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    goto retErr;
		}
		if (tmp)
		    flags |= HALF_HEIGHT;
		break;
	    case O_HALFWIDTH:
		if (Tcl_GetBooleanFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    goto retErr;
		}
		if (tmp)
		    flags |= HALF_WIDTH;
		break;
	    }
	}
    }

    /* convert a statemap into a state table */
    if (objc > 2) {
	Tcl_Obj **specs;
	int n,j,count, status = TCL_OK;
	if (Tcl_ListObjGetElements(interp, objv[2], &count, &specs) != TCL_OK)
	    goto retErr;
	/* we over-allocate to ensure there is a terminating entry */
	stateTable = ckalloc(sizeof(Ttk_StateTable) * (count + 1));
	memset(stateTable, 0, sizeof(Ttk_StateTable) * (count + 1));
	for (n = 0, j = 0; status == TCL_OK && n < count; n += 2, ++j) {
	    Ttk_StateSpec spec = {0,0};
	    status = Ttk_GetStateSpecFromObj(interp, specs[n], &spec);
	    if (status == TCL_OK) {
		stateTable[j].onBits = spec.onbits;
		stateTable[j].offBits = spec.offbits;
		status = Tcl_GetIntFromObj(interp, specs[n+1],
			&stateTable[j].index);
	    }
	}
	if (status != TCL_OK) {
	    ckfree(stateTable);
	    Tcl_DStringFree(&classBuf);
	    return status;
	}
    } else {
	stateTable = ckalloc(sizeof(Ttk_StateTable));
	memset(stateTable, 0, sizeof(Ttk_StateTable));
    }

    elementPtr = ckalloc(sizeof(ElementInfo));
    elementPtr->elementSpec = elementSpec;
    elementPtr->partId = partId;
    elementPtr->statemap = stateTable;
    elementPtr->padding = pad;
    elementPtr->flags = HEAP_ELEMENT | flags;

    /* set the element name to an allocated copy */
    name = ckalloc(strlen(elementName) + 1);
    strcpy(name, elementName);
    elementPtr->elementName = name;

    /* set the class name to an allocated copy */
    wname = ckalloc(Tcl_DStringLength(&classBuf) + sizeof(WCHAR));
    wcscpy(wname, className);
    elementPtr->className = wname;

    elementData = NewElementData(themeData->procs, elementPtr);
    Ttk_RegisterElementSpec(
	theme, elementName, elementPtr->elementSpec, elementData);

    Ttk_RegisterCleanup(interp, elementData, DestroyElementData);
    Tcl_SetObjResult(interp, Tcl_NewStringObj(elementName, -1));
    Tcl_DStringFree(&classBuf);
    return TCL_OK;

retErr:
    Tcl_DStringFree(&classBuf);
    return TCL_ERROR;
}

/*----------------------------------------------------------------------
 * +++ Initialization routine:
 */

MODULE_SCOPE int TtkXPTheme_Init(Tcl_Interp *interp, HWND hwnd)
{
    XPThemeData *themeData;
    XPThemeProcs *procs;
    HINSTANCE hlibrary;
    Ttk_Theme themePtr, parentPtr, vistaPtr;
    ElementInfo *infoPtr;
    OSVERSIONINFOW os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOW);
    GetVersionExW(&os);

    procs = LoadXPThemeProcs(&hlibrary);
    if (!procs)
	return TCL_ERROR;
    procs->stubWindow = hwnd;

    /*
     * Create the new style engine.
     */
    parentPtr = Ttk_GetTheme(interp, "winnative");
    themePtr = Ttk_CreateTheme(interp, "xpnative", parentPtr);

    if (!themePtr)
        return TCL_ERROR;

    /*
     * Set theme data and cleanup proc
     */

    themeData = ckalloc(sizeof(XPThemeData));
    themeData->procs = procs;
    themeData->hlibrary = hlibrary;

    Ttk_SetThemeEnabledProc(themePtr, XPThemeEnabled, themeData);
    Ttk_RegisterCleanup(interp, themeData, XPThemeDeleteProc);
    Ttk_RegisterElementFactory(interp, "vsapi", Ttk_CreateVsapiElement, themeData);

    /*
     * Create the vista theme on suitable platform versions and set the theme
     * enable function. The theme itself is defined in script.
     */

    if (os.dwPlatformId == VER_PLATFORM_WIN32_NT && os.dwMajorVersion > 5) {
	vistaPtr = Ttk_CreateTheme(interp, "vista", themePtr);
	if (vistaPtr) {
	    Ttk_SetThemeEnabledProc(vistaPtr, XPThemeEnabled, themeData);
	}
    }

    /*
     * New elements:
     */
    for (infoPtr = ElementInfoTable; infoPtr->elementName != 0; ++infoPtr) {
	ClientData clientData = NewElementData(procs, infoPtr);
	Ttk_RegisterElementSpec(
	    themePtr, infoPtr->elementName, infoPtr->elementSpec, clientData);
	Ttk_RegisterCleanup(interp, clientData, DestroyElementData);
    }

    Ttk_RegisterElementSpec(themePtr, "Scale.trough", &ttkNullElementSpec, 0);

    /*
     * Layouts:
     */
    Ttk_RegisterLayouts(themePtr, LayoutTable);

    Tcl_PkgProvide(interp, "ttk::theme::xpnative", TTK_VERSION);

    return TCL_OK;
}

#endif /* HAVE_UXTHEME_H */
