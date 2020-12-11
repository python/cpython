/*
 * Copyright (c) 2004, Joe English
 *
 * "classic" theme; implements the classic Motif-like Tk look.
 *
 */

#include "tkInt.h"
#include "ttkTheme.h"

#define DEFAULT_BORDERWIDTH "2"
#define DEFAULT_ARROW_SIZE "15"

/*----------------------------------------------------------------------
 * +++ Highlight element implementation.
 * 	Draw a solid highlight border to indicate focus.
 */

typedef struct {
    Tcl_Obj	*highlightColorObj;
    Tcl_Obj	*highlightThicknessObj;
} HighlightElement;

static Ttk_ElementOptionSpec HighlightElementOptions[] = {
    { "-highlightcolor",TK_OPTION_COLOR,
	Tk_Offset(HighlightElement,highlightColorObj), DEFAULT_BACKGROUND },
    { "-highlightthickness",TK_OPTION_PIXELS,
	Tk_Offset(HighlightElement,highlightThicknessObj), "0" },
    { NULL, 0, 0, NULL }
};

static void HighlightElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    HighlightElement *hl = elementRecord;
    int highlightThickness = 0;

    Tcl_GetIntFromObj(NULL,hl->highlightThicknessObj,&highlightThickness);
    *paddingPtr = Ttk_UniformPadding((short)highlightThickness);
}

static void HighlightElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    HighlightElement *hl = elementRecord;
    int highlightThickness = 0;
    XColor *highlightColor = Tk_GetColorFromObj(tkwin, hl->highlightColorObj);

    Tcl_GetIntFromObj(NULL,hl->highlightThicknessObj,&highlightThickness);
    if (highlightColor && highlightThickness > 0) {
	GC gc = Tk_GCForColor(highlightColor, d);
	Tk_DrawFocusHighlight(tkwin, gc, highlightThickness, d);
    }
}

static Ttk_ElementSpec HighlightElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(HighlightElement),
    HighlightElementOptions,
    HighlightElementSize,
    HighlightElementDraw
};

/*------------------------------------------------------------------------
 * +++ Button Border element:
 * 
 * The Motif-style button border on X11 consists of (from outside-in):
 *
 * + focus indicator (controlled by -highlightcolor and -highlightthickness),
 * + default ring (if -default active; blank if -default normal)
 * + shaded border (controlled by -background, -borderwidth, and -relief)
 */

typedef struct {
    Tcl_Obj	*borderObj;
    Tcl_Obj	*borderWidthObj;
    Tcl_Obj	*reliefObj;
    Tcl_Obj	*defaultStateObj;
} ButtonBorderElement;

static Ttk_ElementOptionSpec ButtonBorderElementOptions[] =
{
    { "-background", TK_OPTION_BORDER, 
	Tk_Offset(ButtonBorderElement,borderObj), DEFAULT_BACKGROUND },
    { "-borderwidth", TK_OPTION_PIXELS, 
	Tk_Offset(ButtonBorderElement,borderWidthObj), DEFAULT_BORDERWIDTH },
    { "-relief", TK_OPTION_RELIEF, 
	Tk_Offset(ButtonBorderElement,reliefObj), "flat" },
    { "-default", TK_OPTION_ANY, 
	Tk_Offset(ButtonBorderElement,defaultStateObj), "disabled" },
    { NULL, 0, 0, NULL }
};

static void ButtonBorderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ButtonBorderElement *bd = elementRecord;
    int defaultState = TTK_BUTTON_DEFAULT_DISABLED;
    int borderWidth = 0;

    Tcl_GetIntFromObj(NULL, bd->borderWidthObj, &borderWidth);
    Ttk_GetButtonDefaultStateFromObj(NULL, bd->defaultStateObj, &defaultState);

    if (defaultState != TTK_BUTTON_DEFAULT_DISABLED) {
	borderWidth += 5;
    }
    *paddingPtr = Ttk_UniformPadding((short)borderWidth);
}

/*
 * (@@@ Note: ButtonBorderElement still still still buggy:
 * padding for default ring is drawn in the wrong color 
 * when the button is active.)
 */
static void ButtonBorderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ButtonBorderElement *bd = elementRecord;
    Tk_3DBorder border = NULL;
    int borderWidth = 1, relief = TK_RELIEF_FLAT;
    int defaultState = TTK_BUTTON_DEFAULT_DISABLED;
    int inset = 0;

    /*
     * Get option values.
     */
    border = Tk_Get3DBorderFromObj(tkwin, bd->borderObj);
    Tcl_GetIntFromObj(NULL, bd->borderWidthObj, &borderWidth);
    Tk_GetReliefFromObj(NULL, bd->reliefObj, &relief);
    Ttk_GetButtonDefaultStateFromObj(NULL, bd->defaultStateObj, &defaultState);

    /*
     * Default ring:
     */
    switch (defaultState)
    {
	case TTK_BUTTON_DEFAULT_DISABLED :
	    break;
	case TTK_BUTTON_DEFAULT_NORMAL :
	    inset += 5;
	    break;
	case TTK_BUTTON_DEFAULT_ACTIVE :
            Tk_Draw3DRectangle(tkwin, d, border,
		b.x+inset, b.y+inset, b.width - 2*inset, b.height - 2*inset,
		2, TK_RELIEF_FLAT);
            inset += 2;
            Tk_Draw3DRectangle(tkwin, d, border,
		b.x+inset, b.y+inset, b.width - 2*inset, b.height - 2*inset,
		1, TK_RELIEF_SUNKEN);
	    ++inset;
            Tk_Draw3DRectangle(tkwin, d, border,
		b.x+inset, b.y+inset, b.width - 2*inset, b.height - 2*inset,
		2, TK_RELIEF_FLAT);
	    inset += 2;
	    break;
    }

    /*
     * 3-D border:
     */
    if (border && borderWidth > 0) {
	Tk_Draw3DRectangle(tkwin, d, border,
	    b.x+inset, b.y+inset, b.width - 2*inset, b.height - 2*inset,
	    borderWidth,relief);
    }
}

static Ttk_ElementSpec ButtonBorderElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(ButtonBorderElement),
    ButtonBorderElementOptions,
    ButtonBorderElementSize,
    ButtonBorderElementDraw
};

/*----------------------------------------------------------------------
 * +++ Arrow element(s).
 *
 * Draws a 3-D shaded triangle.
 * clientData is an enum ArrowDirection pointer.
 */

static int ArrowElements[] = { ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT };
typedef struct
{
    Tcl_Obj *sizeObj;
    Tcl_Obj *borderObj;
    Tcl_Obj *borderWidthObj;
    Tcl_Obj *reliefObj;
} ArrowElement;

static Ttk_ElementOptionSpec ArrowElementOptions[] =
{
    { "-arrowsize", TK_OPTION_PIXELS, Tk_Offset(ArrowElement,sizeObj),
	DEFAULT_ARROW_SIZE },
    { "-background", TK_OPTION_BORDER, Tk_Offset(ArrowElement,borderObj),
    	DEFAULT_BACKGROUND },
    { "-borderwidth", TK_OPTION_PIXELS, Tk_Offset(ArrowElement,borderWidthObj),
    	DEFAULT_BORDERWIDTH },
    { "-relief", TK_OPTION_RELIEF, Tk_Offset(ArrowElement,reliefObj),"raised" },
    { NULL, 0, 0, NULL }
};

static void ArrowElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ArrowElement *arrow = elementRecord;
    int size = 12;

    Tk_GetPixelsFromObj(NULL, tkwin, arrow->sizeObj, &size);
    *widthPtr = *heightPtr = size;
}

static void ArrowElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    int direction = *(int *)clientData;
    ArrowElement *arrow = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, arrow->borderObj);
    int borderWidth = 2;
    int relief = TK_RELIEF_RAISED;
    int size = b.width < b.height ? b.width : b.height;
    XPoint points[3];

    Tk_GetPixelsFromObj(NULL, tkwin, arrow->borderWidthObj, &borderWidth);
    Tk_GetReliefFromObj(NULL, arrow->reliefObj, &relief);


    /*
     * @@@ There are off-by-one pixel errors in the way these are drawn;
     * @@@ need to take a look at Tk_Fill3DPolygon and X11 to find the
     * @@@ exact rules.
     */
    switch (direction)
    {
	case ARROW_UP:
	    points[2].x = b.x; 		points[2].y = b.y + size;
	    points[1].x = b.x + size/2;	points[1].y = b.y;
	    points[0].x = b.x + size;	points[0].y = b.y + size;
	    break;
	case ARROW_DOWN:
	    points[0].x = b.x; 		points[0].y = b.y;
	    points[1].x = b.x + size/2;	points[1].y = b.y + size;
	    points[2].x = b.x + size;	points[2].y = b.y;
	    break;
	case ARROW_LEFT:
	    points[0].x = b.x; 		points[0].y = b.y + size / 2;
	    points[1].x = b.x + size;	points[1].y = b.y + size;
	    points[2].x = b.x + size;	points[2].y = b.y;
	    break;
	case ARROW_RIGHT:
	    points[0].x = b.x + size;	points[0].y = b.y + size / 2;
	    points[1].x = b.x;		points[1].y = b.y;
	    points[2].x = b.x;		points[2].y = b.y + size;
	    break;
    }

    Tk_Fill3DPolygon(tkwin, d, border, points, 3, borderWidth, relief);
}

static Ttk_ElementSpec ArrowElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(ArrowElement),
    ArrowElementOptions,
    ArrowElementSize,
    ArrowElementDraw
};


/*------------------------------------------------------------------------
 * +++ Sash element (for ttk::panedwindow)
 *
 * NOTES: 
 *
 * panedwindows with -orient horizontal use vertical sashes, and vice versa.
 *
 * Interpretation of -sashrelief 'groove' and 'ridge' are
 * swapped wrt. the core panedwindow, which (I think) has them backwards.
 *
 * Default -sashrelief is sunken; the core panedwindow has default 
 * -sashrelief raised, but that looks wrong to me.
 */

static Ttk_Orient SashClientData[] = {
    TTK_ORIENT_HORIZONTAL, TTK_ORIENT_VERTICAL 
};

typedef struct {
    Tcl_Obj *borderObj; 	/* background color */
    Tcl_Obj *sashReliefObj;	/* sash relief */
    Tcl_Obj *sashThicknessObj;	/* overall thickness of sash */
    Tcl_Obj *sashPadObj;	/* padding on either side of handle */
    Tcl_Obj *handleSizeObj;	/* handle width and height */
    Tcl_Obj *handlePadObj;	/* handle's distance from edge */
} SashElement;

static Ttk_ElementOptionSpec SashOptions[] = {
    { "-background", TK_OPTION_BORDER, 
	Tk_Offset(SashElement,borderObj), DEFAULT_BACKGROUND },
    { "-sashrelief", TK_OPTION_RELIEF, 
	Tk_Offset(SashElement,sashReliefObj), "sunken" },
    { "-sashthickness", TK_OPTION_PIXELS,
	Tk_Offset(SashElement,sashThicknessObj), "6" },
    { "-sashpad", TK_OPTION_PIXELS, 
	Tk_Offset(SashElement,sashPadObj), "2" },
    { "-handlesize", TK_OPTION_PIXELS,
	Tk_Offset(SashElement,handleSizeObj), "8" },
    { "-handlepad", TK_OPTION_PIXELS,
	Tk_Offset(SashElement,handlePadObj), "8" },
    { NULL, 0, 0, NULL }
};

static void SashElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SashElement *sash = elementRecord;
    int sashPad = 2, sashThickness = 6, handleSize = 8;
    int horizontal = *((Ttk_Orient*)clientData) == TTK_ORIENT_HORIZONTAL;

    Tk_GetPixelsFromObj(NULL, tkwin, sash->sashThicknessObj, &sashThickness);
    Tk_GetPixelsFromObj(NULL, tkwin, sash->handleSizeObj, &handleSize);
    Tk_GetPixelsFromObj(NULL, tkwin, sash->sashPadObj, &sashPad);

    if (sashThickness < handleSize + 2*sashPad)
	sashThickness = handleSize + 2*sashPad;

    if (horizontal)
	*heightPtr = sashThickness;
    else
	*widthPtr = sashThickness;
}

static void SashElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    SashElement *sash = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, sash->borderObj);
    GC gc1,gc2;
    int relief = TK_RELIEF_RAISED;
    int handleSize = 8, handlePad = 8;
    int horizontal = *((Ttk_Orient*)clientData) == TTK_ORIENT_HORIZONTAL;
    Ttk_Box hb;

    Tk_GetPixelsFromObj(NULL, tkwin, sash->handleSizeObj, &handleSize);
    Tk_GetPixelsFromObj(NULL, tkwin, sash->handlePadObj, &handlePad);
    Tk_GetReliefFromObj(NULL, sash->sashReliefObj, &relief);

    switch (relief) {
	case TK_RELIEF_RAISED: case TK_RELIEF_RIDGE:
	    gc1 = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
	    gc2 = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
	    break;
	case TK_RELIEF_SUNKEN: case TK_RELIEF_GROOVE:
	    gc1 = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
	    gc2 = Tk_3DBorderGC(tkwin, border, TK_3D_LIGHT_GC);
	    break;
	case TK_RELIEF_SOLID: 
	    gc1 = gc2 = Tk_3DBorderGC(tkwin, border, TK_3D_DARK_GC);
	    break;
	case TK_RELIEF_FLAT: 
	default:
	    gc1 = gc2 = Tk_3DBorderGC(tkwin, border, TK_3D_FLAT_GC);
	    break;
    }

    /* Draw sash line:
     */
    if (horizontal) {
	int y = b.y + b.height/2 - 1;
	XDrawLine(Tk_Display(tkwin), d, gc1, b.x, y, b.x+b.width, y); ++y;
	XDrawLine(Tk_Display(tkwin), d, gc2, b.x, y, b.x+b.width, y);
    } else {
	int x = b.x + b.width/2 - 1;
	XDrawLine(Tk_Display(tkwin), d, gc1, x, b.y, x, b.y+b.height); ++x;
	XDrawLine(Tk_Display(tkwin), d, gc2, x, b.y, x, b.y+b.height);
    }

    /* Draw handle:
     */
    if (handleSize >= 0) {
	if (horizontal) {
	    hb = Ttk_StickBox(b, handleSize, handleSize, TTK_STICK_W);
	    hb.x += handlePad;
	} else {
	    hb = Ttk_StickBox(b, handleSize, handleSize, TTK_STICK_N);
	    hb.y += handlePad;
	}
	Tk_Fill3DRectangle(tkwin, d, border, 
	    hb.x, hb.y, hb.width, hb.height, 1, TK_RELIEF_RAISED);
    }
}

static Ttk_ElementSpec SashElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(SashElement),
    SashOptions,
    SashElementSize,
    SashElementDraw
};

/*------------------------------------------------------------------------
 * +++ Widget layouts.
 */

TTK_BEGIN_LAYOUT_TABLE(LayoutTable)

TTK_LAYOUT("TButton",
    TTK_GROUP("Button.highlight", TTK_FILL_BOTH,
        TTK_GROUP("Button.border", TTK_FILL_BOTH|TTK_BORDER,
	    TTK_GROUP("Button.padding", TTK_FILL_BOTH,
	        TTK_NODE("Button.label", TTK_FILL_BOTH)))))

TTK_LAYOUT("TCheckbutton",
    TTK_GROUP("Checkbutton.highlight", TTK_FILL_BOTH,
        TTK_GROUP("Checkbutton.border", TTK_FILL_BOTH,
	    TTK_GROUP("Checkbutton.padding", TTK_FILL_BOTH,
	        TTK_NODE("Checkbutton.indicator", TTK_PACK_LEFT)
	        TTK_NODE("Checkbutton.label", TTK_PACK_LEFT|TTK_FILL_BOTH)))))

TTK_LAYOUT("TRadiobutton",
    TTK_GROUP("Radiobutton.highlight", TTK_FILL_BOTH,
        TTK_GROUP("Radiobutton.border", TTK_FILL_BOTH,
	    TTK_GROUP("Radiobutton.padding", TTK_FILL_BOTH,
	        TTK_NODE("Radiobutton.indicator", TTK_PACK_LEFT)
	        TTK_NODE("Radiobutton.label", TTK_PACK_LEFT|TTK_FILL_BOTH)))))

TTK_LAYOUT("TMenubutton",
    TTK_GROUP("Menubutton.highlight", TTK_FILL_BOTH,
        TTK_GROUP("Menubutton.border", TTK_FILL_BOTH,
	    TTK_NODE("Menubutton.indicator", TTK_PACK_RIGHT)
	    TTK_GROUP("Menubutton.padding", TTK_PACK_LEFT|TTK_EXPAND|TTK_FILL_X,
	        TTK_NODE("Menubutton.label", 0)))))

/* "classic" entry, includes highlight border */
TTK_LAYOUT("TEntry",
    TTK_GROUP("Entry.highlight", TTK_FILL_BOTH,
        TTK_GROUP("Entry.field", TTK_FILL_BOTH|TTK_BORDER,
	    TTK_GROUP("Entry.padding", TTK_FILL_BOTH,
	        TTK_NODE("Entry.textarea", TTK_FILL_BOTH)))))

/* Notebook tabs -- omit focus ring */
TTK_LAYOUT("Tab",
    TTK_GROUP("Notebook.tab", TTK_FILL_BOTH,
	TTK_GROUP("Notebook.padding", TTK_FILL_BOTH,
	    TTK_NODE("Notebook.label", TTK_FILL_BOTH))))

TTK_END_LAYOUT_TABLE

/* POSSIBLY: include Scale layouts w/focus border
 */

/*------------------------------------------------------------------------
 * TtkClassicTheme_Init --
 * 	Install classic theme.
 */

MODULE_SCOPE int TtkClassicTheme_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme =  Ttk_CreateTheme(interp, "classic", NULL);

    if (!theme) {
	return TCL_ERROR;
    }

    /*
     * Register elements:
     */
    Ttk_RegisterElement(interp, theme, "highlight",
	    &HighlightElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "Button.border",
	    &ButtonBorderElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "uparrow",
	    &ArrowElementSpec, &ArrowElements[0]);
    Ttk_RegisterElement(interp, theme, "downarrow",
	    &ArrowElementSpec, &ArrowElements[1]);
    Ttk_RegisterElement(interp, theme, "leftarrow",
	    &ArrowElementSpec, &ArrowElements[2]);
    Ttk_RegisterElement(interp, theme, "rightarrow",
	    &ArrowElementSpec, &ArrowElements[3]);
    Ttk_RegisterElement(interp, theme, "arrow",
	    &ArrowElementSpec, &ArrowElements[0]);

    Ttk_RegisterElement(interp, theme, "hsash", 
	    &SashElementSpec, &SashClientData[0]);
    Ttk_RegisterElement(interp, theme, "vsash",
	    &SashElementSpec, &SashClientData[1]);

    /*
     * Register layouts:
     */
    Ttk_RegisterLayouts(theme, LayoutTable);

    Tcl_PkgProvide(interp, "ttk::theme::classic", TTK_VERSION);

    return TCL_OK;
}

/*EOF*/
