/*
 * Copyright (C) 2004 Joe English
 *
 * "clam" theme; inspired by the XFCE family of Gnome themes.
 */

#include <tk.h>
#include "ttkTheme.h"

/* 
 * Under windows, the Tk-provided XDrawLine and XDrawArc have an 
 * off-by-one error in the end point. This is especially apparent with this
 * theme. Defining this macro as true handles this case.
 */
#if defined(_WIN32) && !defined(WIN32_XDRAWLINE_HACK)
#	define WIN32_XDRAWLINE_HACK 1
#else
#	define WIN32_XDRAWLINE_HACK 0
#endif

#define STR(x) StR(x)
#define StR(x) #x

#define SCROLLBAR_THICKNESS 14

#define FRAME_COLOR	"#dcdad5"
#define LIGHT_COLOR  	"#ffffff"
#define DARK_COLOR  	"#cfcdc8"
#define DARKER_COLOR 	"#bab5ab"
#define DARKEST_COLOR	"#9e9a91"

/*------------------------------------------------------------------------
 * +++ Utilities.
 */

static GC Ttk_GCForColor(Tk_Window tkwin, Tcl_Obj* colorObj, Drawable d)
{
    GC gc = Tk_GCForColor(Tk_GetColorFromObj(tkwin, colorObj), d);

#ifdef MAC_OSX_TK
    /*
     * Workaround for Tk bug under Aqua where the default line width is 0.
     */
    Display *display = Tk_Display(tkwin);
    unsigned long mask = 0ul;
    XGCValues gcValues;

    gcValues.line_width = 1;
    mask = GCLineWidth;

    XChangeGC(display, gc, mask, &gcValues);
#endif

    return gc;
}

static void DrawSmoothBorder(
    Tk_Window tkwin, Drawable d, Ttk_Box b,
    Tcl_Obj *outerColorObj, Tcl_Obj *upperColorObj, Tcl_Obj *lowerColorObj)
{
    Display *display = Tk_Display(tkwin);
    int x1 = b.x, x2 = b.x + b.width - 1;
    int y1 = b.y, y2 = b.y + b.height - 1;
    const int w = WIN32_XDRAWLINE_HACK;
    GC gc;

    if (   outerColorObj
	&& (gc=Ttk_GCForColor(tkwin,outerColorObj,d)))
    {
	XDrawLine(display,d,gc, x1+1,y1, x2-1+w,y1); /* N */
	XDrawLine(display,d,gc, x1+1,y2, x2-1+w,y2); /* S */
	XDrawLine(display,d,gc, x1,y1+1, x1,y2-1+w); /* E */
	XDrawLine(display,d,gc, x2,y1+1, x2,y2-1+w); /* W */
    }

    if (   upperColorObj
	&& (gc=Ttk_GCForColor(tkwin,upperColorObj,d)))
    {
	XDrawLine(display,d,gc, x1+1,y1+1, x2-1+w,y1+1); /* N */
	XDrawLine(display,d,gc, x1+1,y1+1, x1+1,y2-1);   /* E */
    }

    if (   lowerColorObj
	&& (gc=Ttk_GCForColor(tkwin,lowerColorObj,d)))
    {
	XDrawLine(display,d,gc, x2-1,y2-1, x1+1-w,y2-1); /* S */
	XDrawLine(display,d,gc, x2-1,y2-1, x2-1,y1+1-w); /* W */
    }
}

static GC BackgroundGC(Tk_Window tkwin, Tcl_Obj *backgroundObj)
{
    Tk_3DBorder bd = Tk_Get3DBorderFromObj(tkwin, backgroundObj);
    return Tk_3DBorderGC(tkwin, bd, TK_3D_FLAT_GC);
}

/*------------------------------------------------------------------------
 * +++ Border element.
 */

typedef struct {
    Tcl_Obj 	*borderColorObj;
    Tcl_Obj 	*lightColorObj;
    Tcl_Obj 	*darkColorObj;
    Tcl_Obj 	*reliefObj;
    Tcl_Obj 	*borderWidthObj;	/* See <<NOTE-BORDERWIDTH>> */
} BorderElement;

static Ttk_ElementOptionSpec BorderElementOptions[] = {
    { "-bordercolor", TK_OPTION_COLOR,
	Tk_Offset(BorderElement,borderColorObj), DARKEST_COLOR },
    { "-lightcolor", TK_OPTION_COLOR,
	Tk_Offset(BorderElement,lightColorObj), LIGHT_COLOR },
    { "-darkcolor", TK_OPTION_COLOR,
	Tk_Offset(BorderElement,darkColorObj), DARK_COLOR },
    { "-relief", TK_OPTION_RELIEF,
	Tk_Offset(BorderElement,reliefObj), "flat" },
    { "-borderwidth", TK_OPTION_PIXELS,
	Tk_Offset(BorderElement,borderWidthObj), "2" },
    { NULL, 0, 0, NULL }
};

/*
 * <<NOTE-BORDERWIDTH>>: -borderwidth is only partially supported:
 * in this theme, borders are always exactly 2 pixels thick.
 * With -borderwidth 0, border is not drawn at all; 
 * otherwise a 2-pixel border is used.  For -borderwidth > 2, 
 * the excess is used as padding.
 */

static void BorderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    BorderElement *border = (BorderElement*)elementRecord;
    int borderWidth = 2;
    Tk_GetPixelsFromObj(NULL, tkwin, border->borderWidthObj, &borderWidth);
    if (borderWidth == 1) ++borderWidth;
    *paddingPtr = Ttk_UniformPadding((short)borderWidth);
}

static void BorderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    BorderElement *border = elementRecord;
    int relief = TK_RELIEF_FLAT;
    int borderWidth = 2;
    Tcl_Obj *outer = 0, *upper = 0, *lower = 0;

    Tk_GetReliefFromObj(NULL, border->reliefObj, &relief);
    Tk_GetPixelsFromObj(NULL, tkwin, border->borderWidthObj, &borderWidth);

    if (borderWidth == 0) return;

    switch (relief) {
	case TK_RELIEF_GROOVE :
	case TK_RELIEF_RIDGE :
	case TK_RELIEF_RAISED :
	    outer = border->borderColorObj;
	    upper = border->lightColorObj;
	    lower = border->darkColorObj;
	    break;
	case TK_RELIEF_SUNKEN :
	    outer = border->borderColorObj;
	    upper = border->darkColorObj;
	    lower = border->lightColorObj;
	    break;
	case TK_RELIEF_FLAT :
	    outer = upper = lower = 0;
	    break;
	case TK_RELIEF_SOLID :
	    outer = upper = lower = border->borderColorObj;
	    break;
    }

    DrawSmoothBorder(tkwin, d, b, outer, upper, lower);
}

static Ttk_ElementSpec BorderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(BorderElement),
    BorderElementOptions,
    BorderElementSize,
    BorderElementDraw
};

/*------------------------------------------------------------------------
 * +++ Field element.
 */

typedef struct {
    Tcl_Obj 	*borderColorObj;
    Tcl_Obj 	*lightColorObj;
    Tcl_Obj 	*darkColorObj;
    Tcl_Obj 	*backgroundObj;
} FieldElement;

static Ttk_ElementOptionSpec FieldElementOptions[] = {
    { "-bordercolor", TK_OPTION_COLOR,
	Tk_Offset(FieldElement,borderColorObj), DARKEST_COLOR },
    { "-lightcolor", TK_OPTION_COLOR,
	Tk_Offset(FieldElement,lightColorObj), LIGHT_COLOR },
    { "-darkcolor", TK_OPTION_COLOR,
	Tk_Offset(FieldElement,darkColorObj), DARK_COLOR },
    { "-fieldbackground", TK_OPTION_BORDER,
	Tk_Offset(FieldElement,backgroundObj), "white" },
    { NULL, 0, 0, NULL }
};

static void FieldElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *paddingPtr = Ttk_UniformPadding(2);
}

static void FieldElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    FieldElement *field = elementRecord;
    Tk_3DBorder bg = Tk_Get3DBorderFromObj(tkwin, field->backgroundObj);
    Ttk_Box f = Ttk_PadBox(b, Ttk_UniformPadding(2));
    Tcl_Obj *outer = field->borderColorObj,
	    *inner = field->lightColorObj;

    DrawSmoothBorder(tkwin, d, b, outer, inner, inner);
    Tk_Fill3DRectangle(
	tkwin, d, bg, f.x, f.y, f.width, f.height, 0, TK_RELIEF_SUNKEN);
}

static Ttk_ElementSpec FieldElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(FieldElement),
    FieldElementOptions,
    FieldElementSize,
    FieldElementDraw
};

/*
 * Modified field element for comboboxes:
 * 	Right edge is expanded to overlap the dropdown button.
 */
static void ComboboxFieldElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    FieldElement *field = elementRecord;
    GC gc = Ttk_GCForColor(tkwin,field->borderColorObj,d);

    ++b.width;
    FieldElementDraw(clientData, elementRecord, tkwin, d, b, state);

    XDrawLine(Tk_Display(tkwin), d, gc,
	    b.x + b.width - 1, b.y,
	    b.x + b.width - 1, b.y + b.height - 1 + WIN32_XDRAWLINE_HACK);
}

static Ttk_ElementSpec ComboboxFieldElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(FieldElement),
    FieldElementOptions,
    FieldElementSize,
    ComboboxFieldElementDraw
};

/*------------------------------------------------------------------------
 * +++ Indicator elements for check and radio buttons.
 */

typedef struct {
    Tcl_Obj *sizeObj;
    Tcl_Obj *marginObj;
    Tcl_Obj *backgroundObj;
    Tcl_Obj *foregroundObj;
    Tcl_Obj *upperColorObj;
    Tcl_Obj *lowerColorObj;
} IndicatorElement;

static Ttk_ElementOptionSpec IndicatorElementOptions[] = {
    { "-indicatorsize", TK_OPTION_PIXELS,
	Tk_Offset(IndicatorElement,sizeObj), "10" },
    { "-indicatormargin", TK_OPTION_STRING,
	Tk_Offset(IndicatorElement,marginObj), "1" },
    { "-indicatorbackground", TK_OPTION_COLOR,
	Tk_Offset(IndicatorElement,backgroundObj), "white" },
    { "-indicatorforeground", TK_OPTION_COLOR,
	Tk_Offset(IndicatorElement,foregroundObj), "black" },
    { "-upperbordercolor", TK_OPTION_COLOR,
	Tk_Offset(IndicatorElement,upperColorObj), DARKEST_COLOR },
    { "-lowerbordercolor", TK_OPTION_COLOR,
	Tk_Offset(IndicatorElement,lowerColorObj), DARK_COLOR },
    { NULL, 0, 0, NULL }
};

static void IndicatorElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    IndicatorElement *indicator = elementRecord;
    Ttk_Padding margins;
    int size = 10;
    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginObj, &margins);
    Tk_GetPixelsFromObj(NULL, tkwin, indicator->sizeObj, &size);
    *widthPtr = size + Ttk_PaddingWidth(margins);
    *heightPtr = size + Ttk_PaddingHeight(margins);
}

static void RadioIndicatorElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    IndicatorElement *indicator = elementRecord;
    GC gcb=Ttk_GCForColor(tkwin,indicator->backgroundObj,d);
    GC gcf=Ttk_GCForColor(tkwin,indicator->foregroundObj,d);
    GC gcu=Ttk_GCForColor(tkwin,indicator->upperColorObj,d);
    GC gcl=Ttk_GCForColor(tkwin,indicator->lowerColorObj,d);
    Ttk_Padding padding;

    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginObj, &padding);
    b = Ttk_PadBox(b, padding);

    XFillArc(Tk_Display(tkwin),d,gcb, b.x,b.y,b.width,b.height, 0,360*64);
    XDrawArc(Tk_Display(tkwin),d,gcl, b.x,b.y,b.width,b.height, 225*64,180*64);
    XDrawArc(Tk_Display(tkwin),d,gcu, b.x,b.y,b.width,b.height, 45*64,180*64);

    if (state & TTK_STATE_SELECTED) {
	b = Ttk_PadBox(b,Ttk_UniformPadding(3));
	XFillArc(Tk_Display(tkwin),d,gcf, b.x,b.y,b.width,b.height, 0,360*64);
	XDrawArc(Tk_Display(tkwin),d,gcf, b.x,b.y,b.width,b.height, 0,360*64);
#if WIN32_XDRAWLINE_HACK
	XDrawArc(Tk_Display(tkwin),d,gcf, b.x,b.y,b.width,b.height, 300*64,360*64);
#endif
    }
}

static void CheckIndicatorElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    Display *display = Tk_Display(tkwin);
    IndicatorElement *indicator = elementRecord;
    GC gcb=Ttk_GCForColor(tkwin,indicator->backgroundObj,d);
    GC gcf=Ttk_GCForColor(tkwin,indicator->foregroundObj,d);
    GC gcu=Ttk_GCForColor(tkwin,indicator->upperColorObj,d);
    GC gcl=Ttk_GCForColor(tkwin,indicator->lowerColorObj,d);
    Ttk_Padding padding;
    const int w = WIN32_XDRAWLINE_HACK;

    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginObj, &padding);
    b = Ttk_PadBox(b, padding);

    XFillRectangle(display,d,gcb, b.x,b.y,b.width,b.height);
    XDrawLine(display,d,gcl,b.x,b.y+b.height,b.x+b.width+w,b.y+b.height);/*S*/
    XDrawLine(display,d,gcl,b.x+b.width,b.y,b.x+b.width,b.y+b.height+w); /*E*/
    XDrawLine(display,d,gcu,b.x,b.y, b.x,b.y+b.height+w); /*W*/
    XDrawLine(display,d,gcu,b.x,b.y, b.x+b.width+w,b.y);  /*N*/

    if (state & TTK_STATE_SELECTED) {
	int p,q,r,s;

	b = Ttk_PadBox(b,Ttk_UniformPadding(2));
	p = b.x, q = b.y, r = b.x+b.width, s = b.y+b.height;

	r+=w, s+=w;
	XDrawLine(display, d, gcf, p,   q,   r,   s);
	XDrawLine(display, d, gcf, p+1, q,   r,   s-1);
	XDrawLine(display, d, gcf, p,   q+1, r-1, s);

	s-=w, q-=w;
	XDrawLine(display, d, gcf, p,   s,   r,   q);
	XDrawLine(display, d, gcf, p+1, s,   r,   q+1);
	XDrawLine(display, d, gcf, p,   s-1, r-1, q);
    }
}

static Ttk_ElementSpec RadioIndicatorElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(IndicatorElement),
    IndicatorElementOptions,
    IndicatorElementSize,
    RadioIndicatorElementDraw
};

static Ttk_ElementSpec CheckIndicatorElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(IndicatorElement),
    IndicatorElementOptions,
    IndicatorElementSize,
    CheckIndicatorElementDraw
};

#define MENUBUTTON_ARROW_SIZE 5

typedef struct {
    Tcl_Obj *sizeObj;
    Tcl_Obj *colorObj;
    Tcl_Obj *paddingObj;
} MenuIndicatorElement;

static Ttk_ElementOptionSpec MenuIndicatorElementOptions[] =
{
    { "-arrowsize", TK_OPTION_PIXELS,
	Tk_Offset(MenuIndicatorElement,sizeObj), 
	STR(MENUBUTTON_ARROW_SIZE)},
    { "-arrowcolor",TK_OPTION_COLOR,
	Tk_Offset(MenuIndicatorElement,colorObj),
	"black" },
    { "-arrowpadding",TK_OPTION_STRING,
	Tk_Offset(MenuIndicatorElement,paddingObj),
	"3" },
    { NULL, 0, 0, NULL }
};

static void MenuIndicatorElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    MenuIndicatorElement *indicator = elementRecord;
    Ttk_Padding margins;
    int size = MENUBUTTON_ARROW_SIZE;
    Tk_GetPixelsFromObj(NULL, tkwin, indicator->sizeObj, &size);
    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->paddingObj, &margins);
    TtkArrowSize(size, ARROW_DOWN, widthPtr, heightPtr);
    *widthPtr += Ttk_PaddingWidth(margins);
    *heightPtr += Ttk_PaddingHeight(margins);
}

static void MenuIndicatorElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    MenuIndicatorElement *indicator = elementRecord;
    XColor *arrowColor = Tk_GetColorFromObj(tkwin, indicator->colorObj);
    GC gc = Tk_GCForColor(arrowColor, d);
    int size = MENUBUTTON_ARROW_SIZE;
    int width, height;

    Tk_GetPixelsFromObj(NULL, tkwin, indicator->sizeObj, &size);

    TtkArrowSize(size, ARROW_DOWN, &width, &height);
    b = Ttk_StickBox(b, width, height, 0);
    TtkFillArrow(Tk_Display(tkwin), d, gc, b, ARROW_DOWN);
}

static Ttk_ElementSpec MenuIndicatorElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(MenuIndicatorElement),
    MenuIndicatorElementOptions,
    MenuIndicatorElementSize,
    MenuIndicatorElementDraw
};

/*------------------------------------------------------------------------
 * +++ Grips.
 *
 * TODO: factor this with ThumbElementDraw
 */

static Ttk_Orient GripClientData[] = {
    TTK_ORIENT_HORIZONTAL, TTK_ORIENT_VERTICAL
};

typedef struct {
    Tcl_Obj 	*lightColorObj;
    Tcl_Obj 	*borderColorObj;
    Tcl_Obj 	*gripCountObj;
} GripElement;

static Ttk_ElementOptionSpec GripElementOptions[] = {
    { "-lightcolor", TK_OPTION_COLOR,
	Tk_Offset(GripElement,lightColorObj), LIGHT_COLOR },
    { "-bordercolor", TK_OPTION_COLOR,
	Tk_Offset(GripElement,borderColorObj), DARKEST_COLOR },
    { "-gripcount", TK_OPTION_INT,
	Tk_Offset(GripElement,gripCountObj), "5" },
    { NULL, 0, 0, NULL }
};

static void GripElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    int horizontal = *((Ttk_Orient*)clientData) == TTK_ORIENT_HORIZONTAL;
    GripElement *grip = elementRecord;
    int gripCount = 0;

    Tcl_GetIntFromObj(NULL, grip->gripCountObj, &gripCount);
    if (horizontal) {
	*widthPtr = 2*gripCount;
    } else {
	*heightPtr = 2*gripCount;
    }
}

static void GripElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    const int w = WIN32_XDRAWLINE_HACK;
    int horizontal = *((Ttk_Orient*)clientData) == TTK_ORIENT_HORIZONTAL;
    GripElement *grip = elementRecord;
    GC lightGC = Ttk_GCForColor(tkwin,grip->lightColorObj,d);
    GC darkGC = Ttk_GCForColor(tkwin,grip->borderColorObj,d);
    int gripPad = 1, gripCount = 0;
    int i;

    Tcl_GetIntFromObj(NULL, grip->gripCountObj, &gripCount);

    if (horizontal) {
	int x = b.x + b.width / 2 - gripCount;
	int y1 = b.y + gripPad, y2 = b.y + b.height - gripPad - 1 + w;
	for (i=0; i<gripCount; ++i) {
	    XDrawLine(Tk_Display(tkwin), d, darkGC,  x,y1, x,y2); ++x;
	    XDrawLine(Tk_Display(tkwin), d, lightGC, x,y1, x,y2); ++x;
	}
    } else {
	int y = b.y + b.height / 2 - gripCount;
	int x1 = b.x + gripPad, x2 = b.x + b.width - gripPad - 1 + w;
	for (i=0; i<gripCount; ++i) {
	    XDrawLine(Tk_Display(tkwin), d, darkGC,  x1,y, x2,y); ++y;
	    XDrawLine(Tk_Display(tkwin), d, lightGC, x1,y, x2,y); ++y;
	}
    }
}

static Ttk_ElementSpec GripElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(GripElement),
    GripElementOptions,
    GripElementSize,
    GripElementDraw
};

/*------------------------------------------------------------------------
 * +++ Scrollbar elements: trough, arrows, thumb.
 *
 * Notice that the trough element has 0 internal padding;
 * that way the thumb and arrow borders overlap the trough.
 */

typedef struct { /* Common element record for scrollbar elements */
    Tcl_Obj 	*orientObj;
    Tcl_Obj 	*backgroundObj;
    Tcl_Obj 	*borderColorObj;
    Tcl_Obj 	*troughColorObj;
    Tcl_Obj 	*lightColorObj;
    Tcl_Obj 	*darkColorObj;
    Tcl_Obj 	*arrowColorObj;
    Tcl_Obj 	*arrowSizeObj;
    Tcl_Obj 	*gripCountObj;
    Tcl_Obj 	*sliderlengthObj;
} ScrollbarElement;

static Ttk_ElementOptionSpec ScrollbarElementOptions[] = {
    { "-orient", TK_OPTION_ANY,
	Tk_Offset(ScrollbarElement, orientObj), "horizontal" },
    { "-background", TK_OPTION_BORDER,
	Tk_Offset(ScrollbarElement,backgroundObj), FRAME_COLOR },
    { "-bordercolor", TK_OPTION_COLOR,
	Tk_Offset(ScrollbarElement,borderColorObj), DARKEST_COLOR },
    { "-troughcolor", TK_OPTION_COLOR,
	Tk_Offset(ScrollbarElement,troughColorObj), DARKER_COLOR },
    { "-lightcolor", TK_OPTION_COLOR,
	Tk_Offset(ScrollbarElement,lightColorObj), LIGHT_COLOR },
    { "-darkcolor", TK_OPTION_COLOR,
	Tk_Offset(ScrollbarElement,darkColorObj), DARK_COLOR },
    { "-arrowcolor", TK_OPTION_COLOR,
	Tk_Offset(ScrollbarElement,arrowColorObj), "#000000" },
    { "-arrowsize", TK_OPTION_PIXELS,
	Tk_Offset(ScrollbarElement,arrowSizeObj), STR(SCROLLBAR_THICKNESS) },
    { "-gripcount", TK_OPTION_INT,
	Tk_Offset(ScrollbarElement,gripCountObj), "5" },
    { "-sliderlength", TK_OPTION_INT,
	Tk_Offset(ScrollbarElement,sliderlengthObj), "30" },
    { NULL, 0, 0, NULL }
};

static void TroughElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    ScrollbarElement *sb = elementRecord;
    GC gcb = Ttk_GCForColor(tkwin,sb->borderColorObj,d);
    GC gct = Ttk_GCForColor(tkwin,sb->troughColorObj,d);
    XFillRectangle(Tk_Display(tkwin), d, gct, b.x, b.y, b.width-1, b.height-1);
    XDrawRectangle(Tk_Display(tkwin), d, gcb, b.x, b.y, b.width-1, b.height-1);
}

static Ttk_ElementSpec TroughElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ScrollbarElement),
    ScrollbarElementOptions,
    TtkNullElementSize,
    TroughElementDraw
};

static void ThumbElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ScrollbarElement *sb = elementRecord;
    int size = SCROLLBAR_THICKNESS;
    Tcl_GetIntFromObj(NULL, sb->arrowSizeObj, &size);
    *widthPtr = *heightPtr = size;
}

static void ThumbElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    ScrollbarElement *sb = elementRecord;
    int gripCount = 0, orient = TTK_ORIENT_HORIZONTAL;
    GC lightGC, darkGC;
    int x1, y1, x2, y2, dx, dy, i;
    const int w = WIN32_XDRAWLINE_HACK;

    DrawSmoothBorder(tkwin, d, b,
	sb->borderColorObj, sb->lightColorObj, sb->darkColorObj);
    XFillRectangle(
	Tk_Display(tkwin), d, BackgroundGC(tkwin, sb->backgroundObj),
	b.x+2, b.y+2, b.width-4, b.height-4);

    /*
     * Draw grip:
     */
    Ttk_GetOrientFromObj(NULL, sb->orientObj, &orient);
    Tcl_GetIntFromObj(NULL, sb->gripCountObj, &gripCount);
    lightGC = Ttk_GCForColor(tkwin,sb->lightColorObj,d);
    darkGC = Ttk_GCForColor(tkwin,sb->borderColorObj,d);
    
    if (orient == TTK_ORIENT_HORIZONTAL) {
	dx = 1; dy = 0;
	x1 = x2 = b.x + b.width / 2 - gripCount;
	y1 = b.y + 2;
	y2 = b.y + b.height - 3 + w;
    } else {
	dx = 0; dy = 1;
	y1 = y2 = b.y + b.height / 2 - gripCount;
	x1 = b.x + 2;
	x2 = b.x + b.width - 3 + w;
    }

    for (i=0; i<gripCount; ++i) {
	XDrawLine(Tk_Display(tkwin), d, darkGC, x1,y1, x2,y2);
	x1 += dx; x2 += dx; y1 += dy; y2 += dy;
	XDrawLine(Tk_Display(tkwin), d, lightGC, x1,y1, x2,y2);
	x1 += dx; x2 += dx; y1 += dy; y2 += dy;
    }
}

static Ttk_ElementSpec ThumbElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ScrollbarElement),
    ScrollbarElementOptions,
    ThumbElementSize,
    ThumbElementDraw
};

/*------------------------------------------------------------------------
 * +++ Slider element.
 */
static void SliderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ScrollbarElement *sb = elementRecord;
    int length, thickness, orient;

    length = thickness = SCROLLBAR_THICKNESS;
    Ttk_GetOrientFromObj(NULL, sb->orientObj, &orient);
    Tcl_GetIntFromObj(NULL, sb->arrowSizeObj, &thickness);
    Tk_GetPixelsFromObj(NULL, tkwin, sb->sliderlengthObj, &length);
    if (orient == TTK_ORIENT_VERTICAL) {
	*heightPtr = length;
	*widthPtr = thickness;
    } else {
	*heightPtr = thickness;
	*widthPtr = length;
    }

}

static Ttk_ElementSpec SliderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ScrollbarElement),
    ScrollbarElementOptions,
    SliderElementSize,
    ThumbElementDraw
};

/*------------------------------------------------------------------------
 * +++ Progress bar element
 */
static void PbarElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SliderElementSize(clientData, elementRecord, tkwin,
	    widthPtr, heightPtr, paddingPtr);
    *paddingPtr = Ttk_UniformPadding(2);
    *widthPtr += 4;
    *heightPtr += 4;
}

static void PbarElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    ScrollbarElement *sb = elementRecord;
    
    b = Ttk_PadBox(b, Ttk_UniformPadding(2));
    if (b.width > 4 && b.height > 4) {
	DrawSmoothBorder(tkwin, d, b,
	    sb->borderColorObj, sb->lightColorObj, sb->darkColorObj);
	XFillRectangle(Tk_Display(tkwin), d, 
	    BackgroundGC(tkwin, sb->backgroundObj),
	    b.x+2, b.y+2, b.width-4, b.height-4);
    }
}

static Ttk_ElementSpec PbarElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ScrollbarElement),
    ScrollbarElementOptions,
    PbarElementSize,
    PbarElementDraw
};


/*------------------------------------------------------------------------
 * +++ Scrollbar arrows.
 */
static int ArrowElements[] = { ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT };

static void ArrowElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ScrollbarElement *sb = elementRecord;
    int size = SCROLLBAR_THICKNESS;
    Tcl_GetIntFromObj(NULL, sb->arrowSizeObj, &size);
    *widthPtr = *heightPtr = size;
}

static void ArrowElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned state)
{
    ArrowDirection dir = *(ArrowDirection*)clientData;
    ScrollbarElement *sb = elementRecord;
    GC gc = Ttk_GCForColor(tkwin,sb->arrowColorObj, d);
    int h, cx, cy;

    DrawSmoothBorder(tkwin, d, b,
	sb->borderColorObj, sb->lightColorObj, sb->darkColorObj);

    XFillRectangle(
	Tk_Display(tkwin), d, BackgroundGC(tkwin, sb->backgroundObj),
	b.x+2, b.y+2, b.width-4, b.height-4);

    b = Ttk_PadBox(b, Ttk_UniformPadding(3));
    h = b.width < b.height ? b.width : b.height;
    TtkArrowSize(h/2, dir, &cx, &cy);
    b = Ttk_AnchorBox(b, cx, cy, TK_ANCHOR_CENTER);

    TtkFillArrow(Tk_Display(tkwin), d, gc, b, dir);
}

static Ttk_ElementSpec ArrowElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ScrollbarElement),
    ScrollbarElementOptions,
    ArrowElementSize,
    ArrowElementDraw
};


/*------------------------------------------------------------------------
 * +++ Notebook elements.
 * 	
 * Note: Tabs, except for the rightmost, overlap the neighbor to 
 * their right by one pixel.
 */

typedef struct {
    Tcl_Obj *backgroundObj;
    Tcl_Obj *borderColorObj;
    Tcl_Obj *lightColorObj;
    Tcl_Obj *darkColorObj;
} NotebookElement;

static Ttk_ElementOptionSpec NotebookElementOptions[] = {
    { "-background", TK_OPTION_BORDER,
	Tk_Offset(NotebookElement,backgroundObj), FRAME_COLOR },
    { "-bordercolor", TK_OPTION_COLOR,
	Tk_Offset(NotebookElement,borderColorObj), DARKEST_COLOR },
    { "-lightcolor", TK_OPTION_COLOR,
	Tk_Offset(NotebookElement,lightColorObj), LIGHT_COLOR },
    { "-darkcolor", TK_OPTION_COLOR,
	Tk_Offset(NotebookElement,darkColorObj), DARK_COLOR },
    { NULL, 0, 0, NULL }
};

static void TabElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    int borderWidth = 2;
    paddingPtr->top = paddingPtr->left = paddingPtr->right = borderWidth;
    paddingPtr->bottom = 0;
}

static void TabElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    NotebookElement *tab = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, tab->backgroundObj);
    Display *display = Tk_Display(tkwin);
    int borderWidth = 2, dh = 0;
    int x1,y1,x2,y2;
    GC gc;
    const int w = WIN32_XDRAWLINE_HACK;

    if (state & TTK_STATE_SELECTED) {
	dh = borderWidth;
    }

    if (state & TTK_STATE_USER2) {	/* Rightmost tab */
	--b.width;
    }

    Tk_Fill3DRectangle(tkwin, d, border,
	b.x+2, b.y+2, b.width-1, b.height-2+dh, borderWidth, TK_RELIEF_FLAT);

    x1 = b.x, x2 = b.x + b.width;
    y1 = b.y, y2 = b.y + b.height;


    gc=Ttk_GCForColor(tkwin,tab->borderColorObj,d);
    XDrawLine(display,d,gc, x1,y1+1, x1,y2+w);
    XDrawLine(display,d,gc, x2,y1+1, x2,y2+w);
    XDrawLine(display,d,gc, x1+1,y1, x2-1+w,y1);

    gc=Ttk_GCForColor(tkwin,tab->lightColorObj,d);
    XDrawLine(display,d,gc, x1+1,y1+1, x1+1,y2-1+dh+w);
    XDrawLine(display,d,gc, x1+1,y1+1, x2-1+w,y1+1);
}

static Ttk_ElementSpec TabElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NotebookElement),
    NotebookElementOptions,
    TabElementSize,
    TabElementDraw
};

static void ClientElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    int borderWidth = 2;
    *paddingPtr = Ttk_UniformPadding((short)borderWidth);
}

static void ClientElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    NotebookElement *ce = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, ce->backgroundObj);
    int borderWidth = 2;

    Tk_Fill3DRectangle(tkwin, d, border,
	b.x, b.y, b.width, b.height, borderWidth,TK_RELIEF_FLAT);
    DrawSmoothBorder(tkwin, d, b,
    	ce->borderColorObj, ce->lightColorObj, ce->darkColorObj);
}

static Ttk_ElementSpec ClientElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(NotebookElement),
    NotebookElementOptions,
    ClientElementSize,
    ClientElementDraw
};

/*------------------------------------------------------------------------
 * +++ Modified widget layouts.
 */

TTK_BEGIN_LAYOUT_TABLE(LayoutTable)

TTK_LAYOUT("TCombobox",
    TTK_NODE("Combobox.downarrow", TTK_PACK_RIGHT|TTK_FILL_Y)
    TTK_GROUP("Combobox.field", TTK_PACK_LEFT|TTK_FILL_BOTH|TTK_EXPAND,
	TTK_GROUP("Combobox.padding", TTK_FILL_BOTH,
	    TTK_NODE("Combobox.textarea", TTK_FILL_BOTH))))

TTK_LAYOUT("Horizontal.Sash",
    TTK_GROUP("Sash.hsash", TTK_FILL_BOTH,
	TTK_NODE("Sash.hgrip", TTK_FILL_BOTH)))

TTK_LAYOUT("Vertical.Sash",
    TTK_GROUP("Sash.vsash", TTK_FILL_BOTH,
	TTK_NODE("Sash.vgrip", TTK_FILL_BOTH)))

TTK_END_LAYOUT_TABLE

/*------------------------------------------------------------------------
 * +++ Initialization.
 */

MODULE_SCOPE int
TtkClamTheme_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_CreateTheme(interp, "clam", 0);

    if (!theme) {
        return TCL_ERROR;
    }

    Ttk_RegisterElement(interp,
	theme, "border", &BorderElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "field", &FieldElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "Combobox.field", &ComboboxFieldElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "trough", &TroughElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "thumb", &ThumbElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "uparrow", &ArrowElementSpec, &ArrowElements[0]);
    Ttk_RegisterElement(interp,
	theme, "downarrow", &ArrowElementSpec, &ArrowElements[1]);
    Ttk_RegisterElement(interp,
	theme, "leftarrow", &ArrowElementSpec, &ArrowElements[2]);
    Ttk_RegisterElement(interp,
	theme, "rightarrow", &ArrowElementSpec, &ArrowElements[3]);

    Ttk_RegisterElement(interp,
	theme, "Radiobutton.indicator", &RadioIndicatorElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "Checkbutton.indicator", &CheckIndicatorElementSpec, NULL);
    Ttk_RegisterElement(interp,
	theme, "Menubutton.indicator", &MenuIndicatorElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "tab", &TabElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "client", &ClientElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "slider", &SliderElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "bar", &PbarElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "pbar", &PbarElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "hgrip",
	    &GripElementSpec,  &GripClientData[0]);
    Ttk_RegisterElement(interp, theme, "vgrip",
	    &GripElementSpec,  &GripClientData[1]);

    Ttk_RegisterLayouts(theme, LayoutTable);

    Tcl_PkgProvide(interp, "ttk::theme::clam", TTK_VERSION);

    return TCL_OK;
}
