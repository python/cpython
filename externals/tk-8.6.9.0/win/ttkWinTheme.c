/* winTheme.c - Copyright (C) 2004 Pat Thoyts <patthoyts@users.sf.net>
 */

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#endif

#include <tkWinInt.h>

#ifndef DFCS_HOT	/* Windows 98/Me, Windows 200/XP only */
#define DFCS_HOT 0
#endif

#include "ttk/ttkTheme.h"

/*
 * BoxToRect --
 * 	Helper routine.  Converts a Ttk_Box to a Win32 RECT.
 */
static RECT BoxToRect(Ttk_Box b)
{
    RECT rc;
    rc.top = b.y;
    rc.left = b.x;
    rc.bottom = b.y + b.height;
    rc.right = b.x + b.width;
    return rc;
}

/*
 * ReliefToEdge --
 * 	Convert a Tk "relief" value into an Windows "edge" value.
 * 	NB: Caller must check for RELIEF_FLAT and RELIEF_SOLID,
 *	which must be handled specially.
 *
 *	Passing the BF_FLAT flag to DrawEdge() yields something similar
 * 	to TK_RELIEF_SOLID. TK_RELIEF_FLAT can be implemented by not
 *	drawing anything.
 */
static unsigned int ReliefToEdge(int relief)
{
    switch (relief) {
	case TK_RELIEF_RAISED: return EDGE_RAISED;
	case TK_RELIEF_SUNKEN: return EDGE_SUNKEN;
	case TK_RELIEF_RIDGE:  return EDGE_BUMP;
	case TK_RELIEF_GROOVE: return EDGE_ETCHED;
	case TK_RELIEF_SOLID:  return BDR_RAISEDOUTER;
	default:
	case TK_RELIEF_FLAT:   return BDR_RAISEDOUTER;
    }
}

/*------------------------------------------------------------------------
 * +++ State tables for FrameControlElements.
 */

static Ttk_StateTable checkbutton_statemap[] = { /* see also SF#1865898 */
    { DFCS_BUTTON3STATE|DFCS_CHECKED|DFCS_INACTIVE,
    	TTK_STATE_ALTERNATE|TTK_STATE_DISABLED, 0 },
    { DFCS_BUTTON3STATE|DFCS_CHECKED|DFCS_PUSHED,
    	TTK_STATE_ALTERNATE|TTK_STATE_PRESSED, 0 },
    { DFCS_BUTTON3STATE|DFCS_CHECKED|DFCS_HOT,
    	TTK_STATE_ALTERNATE|TTK_STATE_ACTIVE, 0 },
    { DFCS_BUTTON3STATE|DFCS_CHECKED,
    	TTK_STATE_ALTERNATE, 0 },

    { DFCS_CHECKED|DFCS_INACTIVE, TTK_STATE_SELECTED|TTK_STATE_DISABLED, 0 },
    { DFCS_CHECKED|DFCS_PUSHED,   TTK_STATE_SELECTED|TTK_STATE_PRESSED, 0 },
    { DFCS_CHECKED|DFCS_HOT,      TTK_STATE_SELECTED|TTK_STATE_ACTIVE, 0 },
    { DFCS_CHECKED,	          TTK_STATE_SELECTED, 0 },

    { DFCS_INACTIVE, TTK_STATE_DISABLED, 0 },
    { DFCS_PUSHED,   TTK_STATE_PRESSED, 0 },
    { DFCS_HOT,      TTK_STATE_ACTIVE, 0 },
    { 0, 0, 0 },
};

static Ttk_StateTable pushbutton_statemap[] = {
    { DFCS_INACTIVE,	  TTK_STATE_DISABLED, 0 },
    { DFCS_PUSHED,	  TTK_STATE_PRESSED, 0 },
    { DFCS_HOT,		  TTK_STATE_ACTIVE, 0 },
    { 0, 0, 0 }
};

static Ttk_StateTable arrow_statemap[] = {
    { DFCS_INACTIVE,            TTK_STATE_DISABLED, 0 },
    { DFCS_PUSHED | DFCS_FLAT,  TTK_STATE_PRESSED,  0 },
    { 0, 0, 0 }
};

/*------------------------------------------------------------------------
 * +++ FrameControlElement --
 * 	General-purpose element for things drawn with DrawFrameControl
 */
typedef struct {
    const char *name;		/* element name */
    int classId;		/* class id for DrawFrameControl */
    int partId;			/* part id for DrawFrameControl  */
    int cxId;			/* system metric ids for width/height... */
    int cyId;			/* ... or size if FIXEDSIZE bit set */
    Ttk_StateTable *stateMap;	/* map Tk states to Win32 flags */
    Ttk_Padding margins;	/* additional placement padding */
} FrameControlElementData;

#define _FIXEDSIZE  0x80000000L
#define _HALFMETRIC 0x40000000L
#define FIXEDSIZE(id) (id|_FIXEDSIZE)
#define HALFMETRIC(id) (id|_HALFMETRIC)
#define GETMETRIC(m) \
    ((m) & _FIXEDSIZE ? (int)((m) & ~_FIXEDSIZE) : GetSystemMetrics((m)&0x0fffffff))

static FrameControlElementData FrameControlElements[] = {
    { "Checkbutton.indicator",
	DFC_BUTTON, DFCS_BUTTONCHECK, FIXEDSIZE(13), FIXEDSIZE(13),
	checkbutton_statemap, {0,0,4,0} },
    { "Radiobutton.indicator",
    	DFC_BUTTON, DFCS_BUTTONRADIO, FIXEDSIZE(13), FIXEDSIZE(13),
	checkbutton_statemap, {0,0,4,0} },
    { "uparrow",
    	DFC_SCROLL, DFCS_SCROLLUP, SM_CXVSCROLL, SM_CYVSCROLL,
	arrow_statemap, {0,0,0,0} },
    { "downarrow",
    	DFC_SCROLL, DFCS_SCROLLDOWN, SM_CXVSCROLL, SM_CYVSCROLL,
	arrow_statemap, {0,0,0,0} },
    { "leftarrow",
	DFC_SCROLL, DFCS_SCROLLLEFT, SM_CXHSCROLL, SM_CYHSCROLL,
	arrow_statemap, {0,0,0,0} },
    { "rightarrow",
	DFC_SCROLL, DFCS_SCROLLRIGHT, SM_CXHSCROLL, SM_CYHSCROLL,
	arrow_statemap, {0,0,0,0} },
    { "sizegrip",
    	DFC_SCROLL, DFCS_SCROLLSIZEGRIP, SM_CXVSCROLL, SM_CYHSCROLL,
	arrow_statemap, {0,0,0,0} },
    { "Spinbox.uparrow",
	DFC_SCROLL, DFCS_SCROLLUP, SM_CXVSCROLL, HALFMETRIC(SM_CYVSCROLL),
	arrow_statemap, {0,0,0,0} },
    { "Spinbox.downarrow",
	DFC_SCROLL, DFCS_SCROLLDOWN, SM_CXVSCROLL, HALFMETRIC(SM_CYVSCROLL),
	arrow_statemap, {0,0,0,0} },

    { 0,0,0,0,0,0, {0,0,0,0} }
};

/* ---------------------------------------------------------------------- */

static void FrameControlElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    FrameControlElementData *p = clientData;
    int cx = GETMETRIC(p->cxId);
    int cy = GETMETRIC(p->cyId);
    if (p->cxId & _HALFMETRIC) cx /= 2;
    if (p->cyId & _HALFMETRIC) cy /= 2;
    *widthPtr = cx + Ttk_PaddingWidth(p->margins);
    *heightPtr = cy + Ttk_PaddingHeight(p->margins);
}

static void FrameControlElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    FrameControlElementData *elementData = clientData;
    RECT rc = BoxToRect(Ttk_PadBox(b, elementData->margins));
    TkWinDCState dcState;
    HDC hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);

    DrawFrameControl(hdc, &rc,
	elementData->classId,
	elementData->partId|Ttk_StateTableLookup(elementData->stateMap, state));
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec FrameControlElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    FrameControlElementSize,
    FrameControlElementDraw
};

/*----------------------------------------------------------------------
 * +++ Border element implementation.
 */

typedef struct {
    Tcl_Obj	*reliefObj;
} BorderElement;

static Ttk_ElementOptionSpec BorderElementOptions[] = {
    { "-relief",TK_OPTION_RELIEF,Tk_Offset(BorderElement,reliefObj), "flat" },
    {NULL, 0, 0, NULL}
};

static void BorderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    paddingPtr->left = paddingPtr->right = GetSystemMetrics(SM_CXEDGE);
    paddingPtr->top = paddingPtr->bottom = GetSystemMetrics(SM_CYEDGE);
}

static void BorderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    BorderElement *border = elementRecord;
    RECT rc = BoxToRect(b);
    int relief = TK_RELIEF_FLAT;
    TkWinDCState dcState;
    HDC hdc;

    Tk_GetReliefFromObj(NULL, border->reliefObj, &relief);

    if (relief != TK_RELIEF_FLAT) {
	UINT xFlags = (relief == TK_RELIEF_SOLID) ? BF_FLAT : 0;
	hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
	DrawEdge(hdc, &rc, ReliefToEdge(relief), BF_RECT | xFlags);
	TkWinReleaseDrawableDC(d, hdc, &dcState);
    }
}

static Ttk_ElementSpec BorderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(BorderElement),
    BorderElementOptions,
    BorderElementSize,
    BorderElementDraw
};

/*
 * Entry field borders:
 * Sunken border; also fill with window color.
 */

typedef struct {
    Tcl_Obj	*backgroundObj;
} FieldElement;

static Ttk_ElementOptionSpec FieldElementOptions[] = {
    { "-fieldbackground", TK_OPTION_BORDER,
    	Tk_Offset(FieldElement,backgroundObj), "white" },
    { NULL, 0, 0, NULL }
};

static void FieldElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    paddingPtr->left = paddingPtr->right = GetSystemMetrics(SM_CXEDGE);
    paddingPtr->top = paddingPtr->bottom = GetSystemMetrics(SM_CYEDGE);
}

static void FieldElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    FieldElement *field = elementRecord;
    Tk_3DBorder bg = Tk_Get3DBorderFromObj(tkwin, field->backgroundObj);
    RECT rc = BoxToRect(b);
    TkWinDCState dcState;
    HDC hdc;

    Tk_Fill3DRectangle(
	tkwin, d, bg, b.x, b.y, b.width, b.height, 0, TK_RELIEF_FLAT);

    hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec FieldElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(FieldElement),
    FieldElementOptions,
    FieldElementSize,
    FieldElementDraw
};

/*------------------------------------------------------------------------
 * +++ Button borders.
 *	Drawn with DrawFrameControl instead of DrawEdge;
 *	Also draw default indicator and focus ring.
 */
typedef struct {
    Tcl_Obj	*reliefObj;
    Tcl_Obj	*highlightColorObj;
    Tcl_Obj	*defaultStateObj;
} ButtonBorderElement;

static Ttk_ElementOptionSpec ButtonBorderElementOptions[] = {
    { "-relief",TK_OPTION_RELIEF,
	Tk_Offset(ButtonBorderElement,reliefObj), "flat" },
    { "-highlightcolor",TK_OPTION_COLOR,
	Tk_Offset(ButtonBorderElement,highlightColorObj), "black" },
    { "-default", TK_OPTION_ANY,
	Tk_Offset(ButtonBorderElement,defaultStateObj), "disabled" },
    {NULL, 0, 0, NULL}
};

static void ButtonBorderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ButtonBorderElement *bd = elementRecord;
    int relief = TK_RELIEF_RAISED;
    int defaultState = TTK_BUTTON_DEFAULT_DISABLED;
    short int cx, cy;

    Tk_GetReliefFromObj(NULL, bd->reliefObj, &relief);
    Ttk_GetButtonDefaultStateFromObj(NULL, bd->defaultStateObj, &defaultState);
    cx = GetSystemMetrics(SM_CXEDGE);
    cy = GetSystemMetrics(SM_CYEDGE);

    /* Space for default indicator:
     */
    if (defaultState != TTK_BUTTON_DEFAULT_DISABLED) {
    	++cx; ++cy;
    }

    /* Space for focus ring:
     */
    cx += 2;
    cy += 2;

    *paddingPtr = Ttk_MakePadding(cx,cy,cx,cy);
}

static void ButtonBorderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ButtonBorderElement *bd = elementRecord;
    int relief = TK_RELIEF_FLAT;
    int defaultState = TTK_BUTTON_DEFAULT_DISABLED;
    TkWinDCState dcState;
    HDC hdc;
    RECT rc;

    Tk_GetReliefFromObj(NULL, bd->reliefObj, &relief);
    Ttk_GetButtonDefaultStateFromObj(NULL, bd->defaultStateObj, &defaultState);

    if (defaultState == TTK_BUTTON_DEFAULT_ACTIVE) {
	XColor *highlightColor =
	    Tk_GetColorFromObj(tkwin, bd->highlightColorObj);
	GC gc = Tk_GCForColor(highlightColor, d);
	XDrawRectangle(Tk_Display(tkwin), d, gc, b.x,b.y,b.width-1,b.height-1);
    }
    if (defaultState != TTK_BUTTON_DEFAULT_DISABLED) {
	++b.x; ++b.y; b.width -= 2; b.height -= 2;
    }

    hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);

    rc = BoxToRect(b);
    DrawFrameControl(hdc, &rc,
	DFC_BUTTON,	/* classId */
	DFCS_BUTTONPUSH | Ttk_StateTableLookup(pushbutton_statemap, state));

    /* Draw focus ring:
     */
    if (state & TTK_STATE_FOCUS) {
	short int borderWidth = 3;	/* @@@ Use GetSystemMetrics?*/
	rc = BoxToRect(Ttk_PadBox(b, Ttk_UniformPadding(borderWidth)));
    	DrawFocusRect(hdc, &rc);
    }
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec ButtonBorderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ButtonBorderElement),
    ButtonBorderElementOptions,
    ButtonBorderElementSize,
    ButtonBorderElementDraw
};

/*------------------------------------------------------------------------
 * +++ Focus element.
 * 	Draw dashed focus rectangle.
 */

static void FocusElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *paddingPtr = Ttk_UniformPadding(1);
}

static void FocusElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    if (state & TTK_STATE_FOCUS) {
	RECT rc = BoxToRect(b);
	TkWinDCState dcState;
	HDC hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    	DrawFocusRect(hdc, &rc);
	TkWinReleaseDrawableDC(d, hdc, &dcState);
    }
}

static Ttk_ElementSpec FocusElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    FocusElementSize,
    FocusElementDraw
};

/* FillFocusElement --
 * 	Draws a focus ring filled with the selection color
 */

typedef struct {
    Tcl_Obj *fillColorObj;
} FillFocusElement;

static Ttk_ElementOptionSpec FillFocusElementOptions[] = {
    { "-focusfill", TK_OPTION_COLOR,
	Tk_Offset(FillFocusElement,fillColorObj), "white" },
    {NULL, 0, 0, NULL}
};

	/* @@@ FIX THIS */
static void FillFocusElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    FillFocusElement *focus = elementRecord;
    if (state & TTK_STATE_FOCUS) {
	RECT rc = BoxToRect(b);
	TkWinDCState dcState;
	XColor *fillColor = Tk_GetColorFromObj(tkwin, focus->fillColorObj);
	GC gc = Tk_GCForColor(fillColor, d);
	HDC hdc;

	XFillRectangle(Tk_Display(tkwin),d,gc, b.x,b.y,b.width,b.height);
	hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    	DrawFocusRect(hdc, &rc);
	TkWinReleaseDrawableDC(d, hdc, &dcState);
    }
}

/*
 * ComboboxFocusElement --
 * 	Read-only comboboxes have a filled focus ring, editable ones do not.
 */
static void ComboboxFocusElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    if (state & TTK_STATE_READONLY) {
    	FillFocusElementDraw(clientData, elementRecord, tkwin, d, b, state);
    }
}

static Ttk_ElementSpec ComboboxFocusElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(FillFocusElement),
    FillFocusElementOptions,
    FocusElementSize,
    ComboboxFocusElementDraw
};

/*----------------------------------------------------------------------
 * +++ Scrollbar trough element.
 *
 * The native windows scrollbar is drawn using a pattern brush giving a
 * stippled appearance when the trough might otherwise be invisible.
 * We can deal with this here.
 */

typedef struct {	/* clientData for Trough element */
    HBRUSH     PatternBrush;
    HBITMAP    PatternBitmap;
} TroughClientData;

static const WORD Pattern[] = {
    0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa, 0x5555, 0xaaaa
};

static void TroughClientDataDeleteProc(void *clientData)
{
    TroughClientData *cd = clientData;
    DeleteObject(cd->PatternBrush);
    DeleteObject(cd->PatternBitmap);
    ckfree(clientData);
}

static TroughClientData *TroughClientDataInit(Tcl_Interp *interp)
{
    TroughClientData *cd = ckalloc(sizeof(*cd));
    cd->PatternBitmap = CreateBitmap(8, 8, 1, 1, Pattern);
    cd->PatternBrush  = CreatePatternBrush(cd->PatternBitmap);
    Ttk_RegisterCleanup(interp, cd, TroughClientDataDeleteProc);
    return cd;
}

static void TroughElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    TroughClientData *cd = clientData;
    TkWinDCState dcState;
    HDC hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    HBRUSH hbr;
    COLORREF bk, oldbk, oldtxt;

    hbr = SelectObject(hdc, GetSysColorBrush(COLOR_SCROLLBAR));
    bk = GetSysColor(COLOR_3DHIGHLIGHT);
    oldtxt = SetTextColor(hdc, GetSysColor(COLOR_3DFACE));
    oldbk = SetBkColor(hdc, bk);

    /* WAS: if (bk (COLOR_3DHIGHLIGHT) == GetSysColor(COLOR_WINDOW)) ... */
    if (GetSysColor(COLOR_SCROLLBAR) == GetSysColor(COLOR_BTNFACE)) {
	/* Draw using the pattern brush */
	SelectObject(hdc, cd->PatternBrush);
    }

    PatBlt(hdc, b.x, b.y, b.width, b.height, PATCOPY);
    SetBkColor(hdc, oldbk);
    SetTextColor(hdc, oldtxt);
    SelectObject(hdc, hbr);
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec TroughElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    TtkNullElementSize,
    TroughElementDraw
};

/*------------------------------------------------------------------------
 * +++ Thumb element.
 */

typedef struct {
    Tcl_Obj *orientObj;
} ThumbElement;

static Ttk_ElementOptionSpec ThumbElementOptions[] = {
    { "-orient", TK_OPTION_ANY,Tk_Offset(ThumbElement,orientObj),"horizontal"},
    { NULL, 0, 0, NULL }
};

static void ThumbElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ThumbElement *thumbPtr = elementRecord;
    int orient;

    Ttk_GetOrientFromObj(NULL, thumbPtr->orientObj, &orient);
    if (orient == TTK_ORIENT_HORIZONTAL) {
	*widthPtr = GetSystemMetrics(SM_CXHTHUMB);
	*heightPtr = GetSystemMetrics(SM_CYHSCROLL);
    } else {
	*widthPtr = GetSystemMetrics(SM_CXVSCROLL);
	*heightPtr = GetSystemMetrics(SM_CYVTHUMB);
    }
}

static void ThumbElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    RECT rc = BoxToRect(b);
    TkWinDCState dcState;
    HDC hdc;

    /* Windows doesn't show a thumb when the scrollbar is disabled */
    if (state & TTK_STATE_DISABLED)
	return;

    hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec ThumbElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ThumbElement),
    ThumbElementOptions,
    ThumbElementSize,
    ThumbElementDraw
};

/* ----------------------------------------------------------------------
 * The slider element is the shaped thumb used in the slider widget.
 * Windows likes to call this a trackbar.
 */

typedef struct {
    Tcl_Obj *orientObj;  /* orientation of the slider widget */
} SliderElement;

static Ttk_ElementOptionSpec SliderElementOptions[] = {
    { "-orient", TK_OPTION_ANY, Tk_Offset(SliderElement,orientObj),
      "horizontal" },
      { NULL, 0, 0, NULL }
};

static void SliderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SliderElement *slider = elementRecord;
    int orient;

    Ttk_GetOrientFromObj(NULL, slider->orientObj, &orient);
    if (orient == TTK_ORIENT_HORIZONTAL) {
	*widthPtr = (GetSystemMetrics(SM_CXHTHUMB) / 2) | 1;
	*heightPtr = GetSystemMetrics(SM_CYHSCROLL);
    } else {
	*widthPtr = GetSystemMetrics(SM_CXVSCROLL);
	*heightPtr = (GetSystemMetrics(SM_CYVTHUMB) / 2) | 1;
    }
}

static void SliderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    RECT rc = BoxToRect(b);
    TkWinDCState dcState;
    HDC hdc;

    hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_MIDDLE);
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec SliderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(SliderElement),
    SliderElementOptions,
    SliderElementSize,
    SliderElementDraw
};

/*------------------------------------------------------------------------
 * +++ Notebook elements.
 */

static void ClientElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    paddingPtr->left = paddingPtr->right = GetSystemMetrics(SM_CXEDGE);
    paddingPtr->top = paddingPtr->bottom = GetSystemMetrics(SM_CYEDGE);
}

static void ClientElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    RECT rc = BoxToRect(b);
    TkWinDCState dcState;
    HDC hdc = TkWinGetDrawableDC(Tk_Display(tkwin), d, &dcState);
    DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT | BF_SOFT);
    TkWinReleaseDrawableDC(d, hdc, &dcState);
}

static Ttk_ElementSpec ClientElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    ClientElementSize,
    ClientElementDraw
};

/*------------------------------------------------------------------------
 * +++ Layouts.
 */

TTK_BEGIN_LAYOUT_TABLE(LayoutTable)

TTK_LAYOUT("TButton",
    TTK_GROUP("Button.border", TTK_FILL_BOTH,
	TTK_GROUP("Button.padding", TTK_FILL_BOTH,
	    TTK_NODE("Button.label", TTK_FILL_BOTH))))

TTK_LAYOUT("TCombobox",
    TTK_GROUP("Combobox.field", TTK_FILL_BOTH,
	TTK_NODE("Combobox.downarrow", TTK_PACK_RIGHT|TTK_FILL_Y)
	TTK_GROUP("Combobox.padding", TTK_PACK_LEFT|TTK_EXPAND|TTK_FILL_BOTH,
	    TTK_GROUP("Combobox.focus", TTK_PACK_LEFT|TTK_EXPAND|TTK_FILL_BOTH,
		TTK_NODE("Combobox.textarea", TTK_FILL_BOTH)))))

TTK_END_LAYOUT_TABLE

/* ---------------------------------------------------------------------- */

MODULE_SCOPE
int TtkWinTheme_Init(Tcl_Interp *interp, HWND hwnd)
{
    Ttk_Theme themePtr, parentPtr;
    FrameControlElementData *fce = FrameControlElements;

    parentPtr = Ttk_GetTheme(interp, "alt");
    themePtr = Ttk_CreateTheme(interp, "winnative", parentPtr);
    if (!themePtr) {
        return TCL_ERROR;
    }

    Ttk_RegisterElementSpec(themePtr, "border", &BorderElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "Button.border",
	&ButtonBorderElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "field", &FieldElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "focus", &FocusElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "Combobox.focus",
    	&ComboboxFocusElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "thumb", &ThumbElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "slider", &SliderElementSpec, NULL);
    Ttk_RegisterElementSpec(themePtr, "Scrollbar.trough", &TroughElementSpec,
    	TroughClientDataInit(interp));

    Ttk_RegisterElementSpec(themePtr, "client", &ClientElementSpec, NULL);

    for (fce = FrameControlElements; fce->name != 0; ++fce) {
	Ttk_RegisterElementSpec(themePtr, fce->name,
		&FrameControlElementSpec, fce);
    }

    Ttk_RegisterLayouts(themePtr, LayoutTable);

    Tcl_PkgProvide(interp, "ttk::theme::winnative", TTK_VERSION);
    return TCL_OK;
}

