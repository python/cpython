/*
 * Copyright (c) 2003, Joe English
 *
 * Tk alternate theme, intended to match the MSUE and Gtk's (old) default theme
 */

#include "tkInt.h"
#include "ttkTheme.h"

#if defined(_WIN32)
static const int WIN32_XDRAWLINE_HACK = 1;
#else
static const int WIN32_XDRAWLINE_HACK = 0;
#endif

#if defined(MAC_OSX_TK)
  #define IGNORES_VISUAL
#endif

#define BORDERWIDTH     2
#define SCROLLBAR_WIDTH 14
#define MIN_THUMB_SIZE  8

/*
 *----------------------------------------------------------------------
 *
 * Helper routines for border drawing:
 *
 * NOTE: MSUE specifies a slightly different arrangement
 * for button borders than for other elements; "shadowColors"
 * is for button borders.
 *
 * Please excuse the gross misspelling "LITE" for "LIGHT",
 * but it makes things line up nicer.
 */

enum BorderColor { FLAT = 1, LITE = 2, DARK = 3, BRDR = 4 };

/* top-left outer, top-left inner, bottom-right inner, bottom-right outer */
static int const shadowColors[6][4] = {
    { FLAT, FLAT, FLAT, FLAT },	/* TK_RELIEF_FLAT   = 0*/
    { DARK, LITE, DARK, LITE },	/* TK_RELIEF_GROOVE = 1*/
    { LITE, FLAT, DARK, BRDR },	/* TK_RELIEF_RAISED = 2*/
    { LITE, DARK, LITE, DARK },	/* TK_RELIEF_RIDGE  = 3*/
    { BRDR, BRDR, BRDR, BRDR },	/* TK_RELIEF_SOLID  = 4*/
    { BRDR, DARK, FLAT, LITE }	/* TK_RELIEF_SUNKEN = 5*/
};

/* top-left, bottom-right */
static int const thinShadowColors[6][4] = {
    { FLAT, FLAT },	/* TK_RELIEF_FLAT   = 0*/
    { DARK, LITE },	/* TK_RELIEF_GROOVE = 1*/
    { LITE, DARK },	/* TK_RELIEF_RAISED = 2*/
    { LITE, DARK },	/* TK_RELIEF_RIDGE  = 3*/
    { BRDR, BRDR },	/* TK_RELIEF_SOLID  = 4*/
    { DARK, LITE }	/* TK_RELIEF_SUNKEN = 5*/
};

static void DrawCorner(
    Tk_Window tkwin,
    Drawable d,
    Tk_3DBorder border,			/* get most GCs from here... */
    GC borderGC,			/* "window border" color GC */
    int x,int y, int width,int height,	/* where to draw */
    int corner,				/* 0 => top left; 1 => bottom right */
    enum BorderColor color)
{
    XPoint points[3];
    GC gc;

    --width; --height;
    points[0].x = x;			points[0].y = y+height;
    points[1].x = x+width*corner;	points[1].y = y+height*corner;
    points[2].x = x+width;		points[2].y = y;

    if (color == BRDR)
	gc = borderGC;
    else
	gc = Tk_3DBorderGC(tkwin, border, (int)color);

    XDrawLines(Tk_Display(tkwin), d, gc, points, 3, CoordModeOrigin);
}

static void DrawBorder(
    Tk_Window tkwin, Drawable d, Tk_3DBorder border, XColor *borderColor,
    Ttk_Box b, int borderWidth, int relief)
{
    GC borderGC = Tk_GCForColor(borderColor, d);

    switch (borderWidth) {
	case 2: /* "thick" border */
	    DrawCorner(tkwin, d, border, borderGC,
		b.x, b.y, b.width, b.height, 0,shadowColors[relief][0]);
	    DrawCorner(tkwin, d, border, borderGC,
		b.x+1, b.y+1, b.width-2, b.height-2, 0,shadowColors[relief][1]);
	    DrawCorner(tkwin, d, border, borderGC,
		b.x+1, b.y+1, b.width-2, b.height-2, 1,shadowColors[relief][2]);
	    DrawCorner(tkwin, d, border, borderGC,
		b.x, b.y, b.width, b.height, 1,shadowColors[relief][3]);
	    break;
	case 1: /* "thin" border */
	    DrawCorner(tkwin, d, border, borderGC,
		b.x, b.y, b.width, b.height, 0, thinShadowColors[relief][0]);
	    DrawCorner(tkwin, d, border, borderGC,
		b.x, b.y, b.width, b.height, 1, thinShadowColors[relief][1]);
	    break;
	case 0:	/* no border -- do nothing */
	    break;
	default: /* Fall back to Motif-style borders: */
	    Tk_Draw3DRectangle(tkwin, d, border,
		b.x, b.y, b.width, b.height, borderWidth,relief);
	    break;
    }
}

/* Alternate shadow colors for entry fields:
 * NOTE: FLAT color is normally white, and the LITE color is a darker shade.
 */
static int fieldShadowColors[4] = { DARK, BRDR, LITE, FLAT };

static void DrawFieldBorder(
    Tk_Window tkwin, Drawable d, Tk_3DBorder border, XColor *borderColor,
    Ttk_Box b)
{
    GC borderGC = Tk_GCForColor(borderColor, d);
    DrawCorner(tkwin, d, border, borderGC,
	b.x, b.y, b.width, b.height, 0,fieldShadowColors[0]);
    DrawCorner(tkwin, d, border, borderGC,
	b.x+1, b.y+1, b.width-2, b.height-2, 0,fieldShadowColors[1]);
    DrawCorner(tkwin, d, border, borderGC,
	b.x+1, b.y+1, b.width-2, b.height-2, 1,fieldShadowColors[2]);
    DrawCorner(tkwin, d, border, borderGC,
	b.x, b.y, b.width, b.height, 1,fieldShadowColors[3]);
    return;
}

/*
 * ArrowPoints --
 * 	Compute points of arrow polygon.
 */
static void ArrowPoints(Ttk_Box b, ArrowDirection dir, XPoint points[4])
{
    int cx, cy, h;

    switch (dir) {
	case ARROW_UP:
	    h = (b.width - 1)/2;
	    cx = b.x + h;
	    cy = b.y;
	    if (b.height <= h) h = b.height - 1;
	    points[0].x = cx;		points[0].y = cy;
	    points[1].x = cx - h;  	points[1].y = cy + h;
	    points[2].x = cx + h; 	points[2].y = cy + h;
	    break;
	case ARROW_DOWN:
	    h = (b.width - 1)/2;
	    cx = b.x + h;
	    cy = b.y + b.height - 1;
	    if (b.height <= h) h = b.height - 1;
	    points[0].x = cx; 		points[0].y = cy;
	    points[1].x = cx - h;	points[1].y = cy - h;
	    points[2].x = cx + h; 	points[2].y = cy - h;
	    break;
	case ARROW_LEFT:
	    h = (b.height - 1)/2;
	    cx = b.x;
	    cy = b.y + h;
	    if (b.width <= h) h = b.width - 1;
	    points[0].x = cx; 		points[0].y = cy;
	    points[1].x = cx + h;	points[1].y = cy - h;
	    points[2].x = cx + h; 	points[2].y = cy + h;
	    break;
	case ARROW_RIGHT:
	    h = (b.height - 1)/2;
	    cx = b.x + b.width - 1;
	    cy = b.y + h;
	    if (b.width <= h) h = b.width - 1;
	    points[0].x = cx; 		points[0].y = cy;
	    points[1].x = cx - h;	points[1].y = cy - h;
	    points[2].x = cx - h; 	points[2].y = cy + h;
	    break;
    }

    points[3].x = points[0].x;
    points[3].y = points[0].y;
}

/*public*/
void TtkArrowSize(int h, ArrowDirection dir, int *widthPtr, int *heightPtr)
{
    switch (dir) {
	case ARROW_UP:
	case ARROW_DOWN:	*widthPtr = 2*h+1; *heightPtr = h+1; break;
	case ARROW_LEFT:
	case ARROW_RIGHT:	*widthPtr = h+1; *heightPtr = 2*h+1;
    }
}

/*
 * TtkDrawArrow, TtkFillArrow --
 * 	Draw an arrow in the indicated direction inside the specified box.
 */
/*public*/
void TtkFillArrow(
    Display *display, Drawable d, GC gc, Ttk_Box b, ArrowDirection dir)
{
    XPoint points[4];
    ArrowPoints(b, dir, points);
    XFillPolygon(display, d, gc, points, 3, Convex, CoordModeOrigin);
    XDrawLines(display, d, gc, points, 4, CoordModeOrigin);

    /* Work around bug [77527326e5] - ttk artifacts on Ubuntu */
    XDrawPoint(display, d, gc, points[2].x, points[2].y);
}

/*public*/
void TtkDrawArrow(
    Display *display, Drawable d, GC gc, Ttk_Box b, ArrowDirection dir)
{
    XPoint points[4];
    ArrowPoints(b, dir, points);
    XDrawLines(display, d, gc, points, 4, CoordModeOrigin);

    /* Work around bug [77527326e5] - ttk artifacts on Ubuntu */
    XDrawPoint(display, d, gc, points[2].x, points[2].y);
}

/*
 *----------------------------------------------------------------------
 * +++ Border element implementation.
 *
 * This border consists of (from outside-in):
 *
 * + a 1-pixel thick default indicator (defaultable widgets only)
 * + 1- or 2- pixel shaded border (controlled by -background and -relief)
 * + 1 pixel padding (???)
 */

typedef struct {
    Tcl_Obj	*borderObj;
    Tcl_Obj	*borderColorObj;	/* Extra border color */
    Tcl_Obj	*borderWidthObj;
    Tcl_Obj	*reliefObj;
    Tcl_Obj	*defaultStateObj;	/* for buttons */
} BorderElement;

static Ttk_ElementOptionSpec BorderElementOptions[] = {
    { "-background", TK_OPTION_BORDER, Tk_Offset(BorderElement,borderObj),
    	DEFAULT_BACKGROUND },
    { "-bordercolor",TK_OPTION_COLOR,
	Tk_Offset(BorderElement,borderColorObj), "black" },
    { "-default", TK_OPTION_ANY, Tk_Offset(BorderElement,defaultStateObj),
    	"disabled" },
    { "-borderwidth",TK_OPTION_PIXELS,Tk_Offset(BorderElement,borderWidthObj),
    	STRINGIFY(BORDERWIDTH) },
    { "-relief", TK_OPTION_RELIEF, Tk_Offset(BorderElement,reliefObj),
    	"flat" },
        { NULL, 0, 0, NULL }
};

static void BorderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    BorderElement *bd = elementRecord;
    int borderWidth = 0;
    int defaultState = TTK_BUTTON_DEFAULT_DISABLED;

    Tcl_GetIntFromObj(NULL, bd->borderWidthObj, &borderWidth);
    Ttk_GetButtonDefaultStateFromObj(NULL, bd->defaultStateObj, &defaultState);

    if (defaultState != TTK_BUTTON_DEFAULT_DISABLED) {
	++borderWidth;
    }

    *paddingPtr = Ttk_UniformPadding((short)borderWidth);
}

static void BorderElementDraw(
    void *clientData, void *elementRecord,
    Tk_Window tkwin, Drawable d, Ttk_Box b, unsigned int state)
{
    BorderElement *bd = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, bd->borderObj);
    XColor *borderColor = Tk_GetColorFromObj(tkwin, bd->borderColorObj);
    int borderWidth = 2;
    int relief = TK_RELIEF_FLAT;
    int defaultState = TTK_BUTTON_DEFAULT_DISABLED;

    /*
     * Get option values.
     */
    Tcl_GetIntFromObj(NULL, bd->borderWidthObj, &borderWidth);
    Tk_GetReliefFromObj(NULL, bd->reliefObj, &relief);
    Ttk_GetButtonDefaultStateFromObj(NULL, bd->defaultStateObj, &defaultState);

    if (defaultState == TTK_BUTTON_DEFAULT_ACTIVE) {
	GC gc = Tk_GCForColor(borderColor, d);
	XDrawRectangle(Tk_Display(tkwin), d, gc,
		b.x, b.y, b.width-1, b.height-1);
    }
    if (defaultState != TTK_BUTTON_DEFAULT_DISABLED) {
	/* Space for default ring: */
	b = Ttk_PadBox(b, Ttk_UniformPadding(1));
    }

    DrawBorder(tkwin, d, border, borderColor, b, borderWidth, relief);
}

static Ttk_ElementSpec BorderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(BorderElement),
    BorderElementOptions,
    BorderElementSize,
    BorderElementDraw
};

/*----------------------------------------------------------------------
 * +++ Field element:
 * 	Used for editable fields.
 */
typedef struct {
    Tcl_Obj	*borderObj;
    Tcl_Obj	*borderColorObj;	/* Extra border color */
} FieldElement;

static Ttk_ElementOptionSpec FieldElementOptions[] = {
    { "-fieldbackground", TK_OPTION_BORDER, Tk_Offset(FieldElement,borderObj),
    	"white" },
    { "-bordercolor",TK_OPTION_COLOR, Tk_Offset(FieldElement,borderColorObj),
	"black" },
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
    Drawable d, Ttk_Box b, unsigned int state)
{
    FieldElement *field = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, field->borderObj);
    XColor *borderColor = Tk_GetColorFromObj(tkwin, field->borderColorObj);

    Tk_Fill3DRectangle(
	tkwin, d, border, b.x, b.y, b.width, b.height, 0, TK_RELIEF_SUNKEN);
    DrawFieldBorder(tkwin, d, border, borderColor, b);
}

static Ttk_ElementSpec FieldElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(FieldElement),
    FieldElementOptions,
    FieldElementSize,
    FieldElementDraw
};

/*------------------------------------------------------------------------
 * Indicators --
 *
 * 	Code derived (probably incorrectly) from TIP 109 implementation,
 * 	unix/tkUnixButton.c r 1.15.
 */

/*
 * Indicator bitmap descriptor:
 */
typedef struct {
    int width;		/* Width of each image */
    int height;		/* Height of each image */
    int nimages;	/* #images / row */
    const char *const *pixels;	/* array[height] of char[width*nimage] */
    Ttk_StateTable *map;/* used to look up image index by state */
} IndicatorSpec;

#if 0
/*XPM*/
static const char *const button_images[] = {
    /* width height ncolors chars_per_pixel */
    "52 13 8 1",
    /* colors */
    "A c #808000000000 s shadow",
    "B c #000080800000 s highlight",
    "C c #808080800000 s 3dlight",
    "D c #000000008080 s window",
    "E c #808000008080 s 3ddark",
    "F c #000080808080 s frame",
    "G c #000000000000 s foreground",
    "H c #000080800000 s disabledfg",
};
#endif

static Ttk_StateTable checkbutton_states[] = {
    { 0, 0, TTK_STATE_SELECTED|TTK_STATE_DISABLED },
    { 1, TTK_STATE_SELECTED, TTK_STATE_DISABLED },
    { 2, TTK_STATE_DISABLED, TTK_STATE_SELECTED },
    { 3, TTK_STATE_SELECTED|TTK_STATE_DISABLED, 0 },
    { 0, 0, 0 }
};

static const char *const checkbutton_pixels[] = {
    "AAAAAAAAAAAABAAAAAAAAAAAABAAAAAAAAAAAABAAAAAAAAAAAAB",
    "AEEEEEEEEEECBAEEEEEEEEEECBAEEEEEEEEEECBAEEEEEEEEEECB",
    "AEDDDDDDDDDCBAEDDDDDDDDDCBAEFFFFFFFFFCBAEFFFFFFFFFCB",
    "AEDDDDDDDDDCBAEDDDDDDDGDCBAEFFFFFFFFFCBAEFFFFFFFHFCB",
    "AEDDDDDDDDDCBAEDDDDDDGGDCBAEFFFFFFFFFCBAEFFFFFFHHFCB",
    "AEDDDDDDDDDCBAEDGDDDGGGDCBAEFFFFFFFFFCBAEFHFFFHHHFCB",
    "AEDDDDDDDDDCBAEDGGDGGGDDCBAEFFFFFFFFFCBAEFHHFHHHFFCB",
    "AEDDDDDDDDDCBAEDGGGGGDDDCBAEFFFFFFFFFCBAEFHHHHHFFFCB",
    "AEDDDDDDDDDCBAEDDGGGDDDDCBAEFFFFFFFFFCBAEFFHHHFFFFCB",
    "AEDDDDDDDDDCBAEDDDGDDDDDCBAEFFFFFFFFFCBAEFFFHFFFFFCB",
    "AEDDDDDDDDDCBAEDDDDDDDDDCBAEFFFFFFFFFCBAEFFFFFFFFFCB",
    "ACCCCCCCCCCCBACCCCCCCCCCCBACCCCCCCCCCCBACCCCCCCCCCCB",
    "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB",
};

static IndicatorSpec checkbutton_spec = {
    13, 13, 4,		/* width, height, nimages */
    checkbutton_pixels,
    checkbutton_states
};

static Ttk_StateTable radiobutton_states[] = {
    { 0, 0, TTK_STATE_SELECTED|TTK_STATE_DISABLED },
    { 1, TTK_STATE_SELECTED, TTK_STATE_DISABLED },
    { 2, TTK_STATE_DISABLED, TTK_STATE_SELECTED },
    { 3, TTK_STATE_SELECTED|TTK_STATE_DISABLED, 0 },
    { 0, 0, 0 }
};

static const char *const radiobutton_pixels[] = {
    "FFFFAAAAFFFFFFFFFAAAAFFFFFFFFFAAAAFFFFFFFFFAAAAFFFFF",
    "FFAAEEEEAAFFFFFAAEEEEAAFFFFFAAEEEEAAFFFFFAAEEEEAAFFF",
    "FAEEDDDDEEBFFFAEEDDDDEEBFFFAEEFFFFEEBFFFAEEFFFFEEBFF",
    "FAEDDDDDDCBFFFAEDDDDDDCBFFFAEFFFFFFCBFFFAEFFFFFFCBFF",
    "AEDDDDDDDDCBFAEDDDGGDDDCBFAEFFFFFFFFCBFAEFFFHHFFFCBF",
    "AEDDDDDDDDCBFAEDDGGGGDDCBFAEFFFFFFFFCBFAEFFHHHHFFCBF",
    "AEDDDDDDDDCBFAEDDGGGGDDCBFAEFFFFFFFFCBFAEFFHHHHFFCBF",
    "AEDDDDDDDDCBFAEDDDGGDDDCBFAEFFFFFFFFCBFAEFFFHHFFFCBF",
    "FAEDDDDDDCBFFFAEDDDDDDCBFFFAEFFFFFFCBFFFAEFFFFFFCBFF",
    "FACCDDDDCCBFFFACCDDDDCCBFFFACCFFFFCCBFFFACCFFFFCCBFF",
    "FFBBCCCCBBFFFFFBBCCCCBBFFFFFBBCCCCBBFFFFFBBCCCCBBFFF",
    "FFFFBBBBFFFFFFFFFBBBBFFFFFFFFFBBBBFFFFFFFFFBBBBFFFFF",
    "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF",
};

static IndicatorSpec radiobutton_spec = {
    13, 13, 4,		/* width, height, nimages */
    radiobutton_pixels,
    radiobutton_states
};

typedef struct {
    Tcl_Obj *backgroundObj;
    Tcl_Obj *foregroundObj;
    Tcl_Obj *colorObj;
    Tcl_Obj *lightColorObj;
    Tcl_Obj *shadeColorObj;
    Tcl_Obj *borderColorObj;
    Tcl_Obj *marginObj;
} IndicatorElement;

static Ttk_ElementOptionSpec IndicatorElementOptions[] = {
    { "-background", TK_OPTION_COLOR,
	    Tk_Offset(IndicatorElement,backgroundObj), DEFAULT_BACKGROUND },
    { "-foreground", TK_OPTION_COLOR,
	    Tk_Offset(IndicatorElement,foregroundObj), DEFAULT_FOREGROUND },
    { "-indicatorcolor", TK_OPTION_COLOR,
	    Tk_Offset(IndicatorElement,colorObj), "#FFFFFF" },
    { "-lightcolor", TK_OPTION_COLOR,
	    Tk_Offset(IndicatorElement,lightColorObj), "#DDDDDD" },
    { "-shadecolor", TK_OPTION_COLOR,
	    Tk_Offset(IndicatorElement,shadeColorObj), "#888888" },
    { "-bordercolor", TK_OPTION_COLOR,
	    Tk_Offset(IndicatorElement,borderColorObj), "black" },
    { "-indicatormargin", TK_OPTION_STRING,
	    Tk_Offset(IndicatorElement,marginObj), "0 2 4 2" },
	    { NULL, 0, 0, NULL }
};

static void IndicatorElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    IndicatorSpec *spec = clientData;
    IndicatorElement *indicator = elementRecord;
    Ttk_Padding margins;
    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginObj, &margins);
    *widthPtr = spec->width + Ttk_PaddingWidth(margins);
    *heightPtr = spec->height + Ttk_PaddingHeight(margins);
}

static void IndicatorElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    IndicatorSpec *spec = clientData;
    IndicatorElement *indicator = elementRecord;
    Display *display = Tk_Display(tkwin);
    Ttk_Padding padding;
    XColor *fgColor, *frameColor, *shadeColor, *indicatorColor, *borderColor;

    int index, ix, iy;
    XGCValues gcValues;
    GC copyGC;
    unsigned long imgColors[8];
    XImage *img = NULL;

    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginObj, &padding);
    b = Ttk_PadBox(b, padding);

    if (   b.x < 0
	|| b.y < 0
	|| Tk_Width(tkwin) < b.x + spec->width
	|| Tk_Height(tkwin) < b.y + spec->height)
    {
	/* Oops!  not enough room to display the image.
	 * Don't draw anything.
	 */
	return;
    }

    /*
     * Fill in imgColors palette:
     *
     * (SHOULD: take light and shade colors from the border object,
     * but Tk doesn't provide easy access to these in the public API.)
     */
    fgColor = Tk_GetColorFromObj(tkwin, indicator->foregroundObj);
    frameColor = Tk_GetColorFromObj(tkwin, indicator->backgroundObj);
    shadeColor = Tk_GetColorFromObj(tkwin, indicator->shadeColorObj);
    indicatorColor = Tk_GetColorFromObj(tkwin, indicator->colorObj);
    borderColor = Tk_GetColorFromObj(tkwin, indicator->borderColorObj);

    imgColors[0 /*A*/] = shadeColor->pixel;
    imgColors[1 /*B*/] = indicatorColor->pixel;
    imgColors[2 /*C*/] = frameColor->pixel;
    imgColors[3 /*D*/] = indicatorColor->pixel;
    imgColors[4 /*E*/] = borderColor->pixel;
    imgColors[5 /*F*/] = frameColor->pixel;
    imgColors[6 /*G*/] = fgColor->pixel;
    imgColors[7 /*H*/] = fgColor->pixel;

    /*
     * Create a scratch buffer to store the image:
     */

#if defined(IGNORES_VISUAL)

    /*
     * Platforms which ignore the VisualInfo can use XCreateImage to get the
     * scratch image.  This is essential on macOS, where it is not safe to call
     * XGetImage in a display procedure.
     */

    img = XCreateImage(display, NULL, 32, ZPixmap, 0, NULL,
		       (unsigned int)spec->width, (unsigned int)spec->height,
		       0, 0);
#else

    /*
     * This trick allows creating the scratch XImage without having to
     * construct a VisualInfo.
     */

    img = XGetImage(display, d, 0, 0,
		    (unsigned int)spec->width, (unsigned int)spec->height,
		    AllPlanes, ZPixmap);
#endif

    if (img == NULL) {
        return;
    }

#if defined(IGNORES_VISUAL)

    img->data = ckalloc(img->bytes_per_line * img->height);
    if (img->data == NULL) {
        XDestroyImage(img);
	return;
    }

#endif

    /*
     * Create the image, painting it into the XImage one pixel at a time.
     */

    index = Ttk_StateTableLookup(spec->map, state);
    for (iy=0 ; iy<spec->height ; iy++) {
	for (ix=0 ; ix<spec->width ; ix++) {
	    XPutPixel(img, ix, iy,
		imgColors[spec->pixels[iy][index*spec->width+ix] - 'A'] );
	}
    }

    /*
     * Copy the image onto our target drawable surface.
     */

    memset(&gcValues, 0, sizeof(gcValues));
    copyGC = Tk_GetGC(tkwin, 0, &gcValues);
    TkPutImage(NULL, 0, display, d, copyGC, img, 0, 0, b.x, b.y,
               spec->width, spec->height);

    /*
     * Tidy up.
     */

    Tk_FreeGC(display, copyGC);

    /*
     * Protect against the possibility that some future platform might
     * not use the Tk memory manager in its implementation of XDestroyImage,
     * even though that would be an extremely strange thing to do.
     */

#if defined(IGNORES_VISUAL)
    ckfree(img->data);
    img->data = NULL;
#endif

    XDestroyImage(img);
}

static Ttk_ElementSpec IndicatorElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(IndicatorElement),
    IndicatorElementOptions,
    IndicatorElementSize,
    IndicatorElementDraw
};

/*----------------------------------------------------------------------
 * +++ Arrow element(s).
 *
 * 	Draws a solid triangle, inside a box.
 * 	clientData is an enum ArrowDirection pointer.
 */

static int ArrowElements[] = { ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT };
typedef struct {
    Tcl_Obj *sizeObj;
    Tcl_Obj *borderObj;
    Tcl_Obj *borderColorObj;	/* Extra color for borders */
    Tcl_Obj *reliefObj;
    Tcl_Obj *colorObj;		/* Arrow color */
} ArrowElement;

static Ttk_ElementOptionSpec ArrowElementOptions[] = {
    { "-arrowsize", TK_OPTION_PIXELS,
	Tk_Offset(ArrowElement,sizeObj), STRINGIFY(SCROLLBAR_WIDTH) },
    { "-background", TK_OPTION_BORDER,
	Tk_Offset(ArrowElement,borderObj), DEFAULT_BACKGROUND },
    { "-bordercolor", TK_OPTION_COLOR,
	Tk_Offset(ArrowElement,borderColorObj), "black" },
    { "-relief", TK_OPTION_RELIEF,
	Tk_Offset(ArrowElement,reliefObj),"raised"},
    { "-arrowcolor", TK_OPTION_COLOR,
	Tk_Offset(ArrowElement,colorObj),"black"},
    { NULL, 0, 0, NULL }
};

/*
 * Note asymmetric padding:
 * top/left padding is 1 less than bottom/right,
 * since in this theme 2-pixel borders are asymmetric.
 */
static Ttk_Padding ArrowPadding = { 3,3,4,4 };

static void ArrowElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ArrowElement *arrow = elementRecord;
    int direction = *(int *)clientData;
    int width = SCROLLBAR_WIDTH;

    Tk_GetPixelsFromObj(NULL, tkwin, arrow->sizeObj, &width);
    width -= Ttk_PaddingWidth(ArrowPadding);
    TtkArrowSize(width/2, direction, widthPtr, heightPtr);
    *widthPtr += Ttk_PaddingWidth(ArrowPadding);
    *heightPtr += Ttk_PaddingHeight(ArrowPadding);
}

static void ArrowElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    int direction = *(int *)clientData;
    ArrowElement *arrow = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, arrow->borderObj);
    XColor *borderColor = Tk_GetColorFromObj(tkwin, arrow->borderColorObj);
    XColor *arrowColor = Tk_GetColorFromObj(tkwin, arrow->colorObj);
    int relief = TK_RELIEF_RAISED;
    int borderWidth = 2;

    Tk_GetReliefFromObj(NULL, arrow->reliefObj, &relief);

    Tk_Fill3DRectangle(
	tkwin, d, border, b.x, b.y, b.width, b.height, 0, TK_RELIEF_FLAT);
    DrawBorder(tkwin,d,border,borderColor,b,borderWidth,relief);

    TtkFillArrow(Tk_Display(tkwin), d, Tk_GCForColor(arrowColor, d),
	Ttk_PadBox(b, ArrowPadding), direction);
}

static Ttk_ElementSpec ArrowElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ArrowElement),
    ArrowElementOptions,
    ArrowElementSize,
    ArrowElementDraw
};

/*----------------------------------------------------------------------
 * +++ Menubutton indicator:
 * 	Draw an arrow in the direction where the menu will be posted.
 */

#define MENUBUTTON_ARROW_SIZE 5

typedef struct {
    Tcl_Obj *directionObj;
    Tcl_Obj *sizeObj;
    Tcl_Obj *colorObj;
} MenubuttonArrowElement;

static const char *directionStrings[] = {	/* See also: button.c */
    "above", "below", "left", "right", "flush", NULL
};
enum { POST_ABOVE, POST_BELOW, POST_LEFT, POST_RIGHT, POST_FLUSH };

static Ttk_ElementOptionSpec MenubuttonArrowElementOptions[] = {
    { "-direction", TK_OPTION_STRING,
	Tk_Offset(MenubuttonArrowElement,directionObj), "below" },
    { "-arrowsize", TK_OPTION_PIXELS,
	Tk_Offset(MenubuttonArrowElement,sizeObj), STRINGIFY(MENUBUTTON_ARROW_SIZE)},
    { "-arrowcolor",TK_OPTION_COLOR,
	Tk_Offset(MenubuttonArrowElement,colorObj), "black"},
    { NULL, 0, 0, NULL }
};

static Ttk_Padding MenubuttonArrowPadding = { 3, 0, 3, 0 };

static void MenubuttonArrowElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    MenubuttonArrowElement *arrow = elementRecord;
    int size = MENUBUTTON_ARROW_SIZE;
    Tk_GetPixelsFromObj(NULL, tkwin, arrow->sizeObj, &size);
    *widthPtr = *heightPtr = 2 * size + 1;
    *widthPtr += Ttk_PaddingWidth(MenubuttonArrowPadding);
    *heightPtr += Ttk_PaddingHeight(MenubuttonArrowPadding);
}

static void MenubuttonArrowElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    MenubuttonArrowElement *arrow = elementRecord;
    XColor *arrowColor = Tk_GetColorFromObj(tkwin, arrow->colorObj);
    GC gc = Tk_GCForColor(arrowColor, d);
    int size = MENUBUTTON_ARROW_SIZE;
    int postDirection = POST_BELOW;
    ArrowDirection arrowDirection = ARROW_DOWN;
    int width = 0, height = 0;

    Tk_GetPixelsFromObj(NULL, tkwin, arrow->sizeObj, &size);
    Tcl_GetIndexFromObjStruct(NULL, arrow->directionObj, directionStrings,
	   sizeof(char *), ""/*message*/, 0/*flags*/, &postDirection);

    /* ... this might not be such a great idea ... */
    switch (postDirection) {
	case POST_ABOVE:	arrowDirection = ARROW_UP; break;
	case POST_BELOW:	arrowDirection = ARROW_DOWN; break;
	case POST_LEFT:		arrowDirection = ARROW_LEFT; break;
	case POST_RIGHT:	arrowDirection = ARROW_RIGHT; break;
	case POST_FLUSH:	arrowDirection = ARROW_DOWN; break;
    }

    TtkArrowSize(size, arrowDirection, &width, &height);
    b = Ttk_PadBox(b, MenubuttonArrowPadding);
    b = Ttk_AnchorBox(b, width, height, TK_ANCHOR_CENTER);
    TtkFillArrow(Tk_Display(tkwin), d, gc, b, arrowDirection);
}

static Ttk_ElementSpec MenubuttonArrowElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(MenubuttonArrowElement),
    MenubuttonArrowElementOptions,
    MenubuttonArrowElementSize,
    MenubuttonArrowElementDraw
};

/*----------------------------------------------------------------------
 * +++ Trough element
 *
 * Used in scrollbars and the scale.
 *
 * The -groovewidth option can be used to set the size of the short axis
 * for the drawn area. This will not affect the geometry, but can be used
 * to draw a thin centered trough inside the packet alloted. This is used
 * to show a win32-style scale widget. Use -1 or a large number to use the
 * full area (default).
 *
 */

typedef struct {
    Tcl_Obj *colorObj;
    Tcl_Obj *borderWidthObj;
    Tcl_Obj *reliefObj;
    Tcl_Obj *grooveWidthObj;
    Tcl_Obj *orientObj;
} TroughElement;

static Ttk_ElementOptionSpec TroughElementOptions[] = {
    { "-orient", TK_OPTION_ANY,
	Tk_Offset(TroughElement, orientObj), "horizontal" },
    { "-troughborderwidth", TK_OPTION_PIXELS,
	Tk_Offset(TroughElement,borderWidthObj), "1" },
    { "-troughcolor", TK_OPTION_BORDER,
	Tk_Offset(TroughElement,colorObj), DEFAULT_BACKGROUND },
    { "-troughrelief",TK_OPTION_RELIEF,
	Tk_Offset(TroughElement,reliefObj), "sunken" },
    { "-groovewidth", TK_OPTION_PIXELS,
	Tk_Offset(TroughElement,grooveWidthObj), "-1" },
    { NULL, 0, 0, NULL }
};

static void TroughElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    TroughElement *troughPtr = elementRecord;
    int borderWidth = 2, grooveWidth = 0;

    Tk_GetPixelsFromObj(NULL, tkwin, troughPtr->borderWidthObj, &borderWidth);
    Tk_GetPixelsFromObj(NULL, tkwin, troughPtr->grooveWidthObj, &grooveWidth);

    if (grooveWidth <= 0) {
	*paddingPtr = Ttk_UniformPadding((short)borderWidth);
    }
}

static void TroughElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    TroughElement *troughPtr = elementRecord;
    Tk_3DBorder border = NULL;
    int borderWidth = 2, relief = TK_RELIEF_SUNKEN, groove = -1, orient;

    border = Tk_Get3DBorderFromObj(tkwin, troughPtr->colorObj);
    Ttk_GetOrientFromObj(NULL, troughPtr->orientObj, &orient);
    Tk_GetReliefFromObj(NULL, troughPtr->reliefObj, &relief);
    Tk_GetPixelsFromObj(NULL, tkwin, troughPtr->borderWidthObj, &borderWidth);
    Tk_GetPixelsFromObj(NULL, tkwin, troughPtr->grooveWidthObj, &groove);

    if (groove != -1 && groove < b.height && groove < b.width) {
	if (orient == TTK_ORIENT_HORIZONTAL) {
	    b.y = b.y + b.height/2 - groove/2;
	    b.height = groove;
	} else {
	    b.x = b.x + b.width/2 - groove/2;
	    b.width = groove;
	}
    }

    Tk_Fill3DRectangle(tkwin, d, border, b.x, b.y, b.width, b.height,
	    borderWidth, relief);
}

static Ttk_ElementSpec TroughElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(TroughElement),
    TroughElementOptions,
    TroughElementSize,
    TroughElementDraw
};

/*
 *----------------------------------------------------------------------
 * +++ Thumb element.
 */

typedef struct {
    Tcl_Obj *sizeObj;
    Tcl_Obj *firstObj;
    Tcl_Obj *lastObj;
    Tcl_Obj *borderObj;
    Tcl_Obj *borderColorObj;
    Tcl_Obj *reliefObj;
    Tcl_Obj *orientObj;
} ThumbElement;

static Ttk_ElementOptionSpec ThumbElementOptions[] = {
    { "-width", TK_OPTION_PIXELS, Tk_Offset(ThumbElement,sizeObj),
        STRINGIFY(SCROLLBAR_WIDTH) },
    { "-background", TK_OPTION_BORDER, Tk_Offset(ThumbElement,borderObj),
	DEFAULT_BACKGROUND },
    { "-bordercolor", TK_OPTION_COLOR, Tk_Offset(ThumbElement,borderColorObj),
	"black" },
    { "-relief", TK_OPTION_RELIEF,Tk_Offset(ThumbElement,reliefObj),"raised" },
    { "-orient", TK_OPTION_ANY,Tk_Offset(ThumbElement,orientObj),"horizontal"},
    { NULL, 0, 0, NULL }
};

static void ThumbElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ThumbElement *thumb = elementRecord;
    int orient, size;
    Tk_GetPixelsFromObj(NULL, tkwin, thumb->sizeObj, &size);
    Ttk_GetOrientFromObj(NULL, thumb->orientObj, &orient);

    if (orient == TTK_ORIENT_VERTICAL) {
	*widthPtr = size;
	*heightPtr = MIN_THUMB_SIZE;
    } else {
	*widthPtr = MIN_THUMB_SIZE;
	*heightPtr = size;
    }
}

static void ThumbElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    ThumbElement *thumb = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, thumb->borderObj);
    XColor *borderColor = Tk_GetColorFromObj(tkwin, thumb->borderColorObj);
    int relief = TK_RELIEF_RAISED;
    int borderWidth = 2;

    /*
     * Don't draw the thumb if we are disabled.
     * This makes it behave like Windows ... if that's what we want.
    if (state & TTK_STATE_DISABLED)
	return;
     */

    Tk_GetReliefFromObj(NULL, thumb->reliefObj, &relief);

    Tk_Fill3DRectangle(
	tkwin, d, border, b.x,b.y,b.width,b.height, 0, TK_RELIEF_FLAT);
    DrawBorder(tkwin, d, border, borderColor, b, borderWidth, relief);
}

static Ttk_ElementSpec ThumbElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ThumbElement),
    ThumbElementOptions,
    ThumbElementSize,
    ThumbElementDraw
};

/*
 *----------------------------------------------------------------------
 * +++ Slider element.
 *
 * This is the moving part of the scale widget.
 *
 * The slider element is the thumb in the scale widget. This is drawn
 * as an arrow-type element that can point up, down, left or right.
 *
 */

typedef struct {
    Tcl_Obj *lengthObj;		/* Long axis dimension */
    Tcl_Obj *thicknessObj;	/* Short axis dimension */
    Tcl_Obj *reliefObj;		/* Relief for this object */
    Tcl_Obj *borderObj;		/* Border / background color */
    Tcl_Obj *borderColorObj;	/* Additional border color */
    Tcl_Obj *borderWidthObj;
    Tcl_Obj *orientObj;		/* Orientation of overall slider */
} SliderElement;

static Ttk_ElementOptionSpec SliderElementOptions[] = {
    { "-sliderlength", TK_OPTION_PIXELS, Tk_Offset(SliderElement,lengthObj),
	"15" },
    { "-sliderthickness",TK_OPTION_PIXELS,Tk_Offset(SliderElement,thicknessObj),
	"15" },
    { "-sliderrelief", TK_OPTION_RELIEF, Tk_Offset(SliderElement,reliefObj),
	"raised" },
    { "-borderwidth", TK_OPTION_PIXELS, Tk_Offset(SliderElement,borderWidthObj),
	STRINGIFY(BORDERWIDTH) },
    { "-background", TK_OPTION_BORDER, Tk_Offset(SliderElement,borderObj),
	DEFAULT_BACKGROUND },
    { "-bordercolor", TK_OPTION_COLOR, Tk_Offset(ThumbElement,borderColorObj),
	"black" },
    { "-orient", TK_OPTION_ANY, Tk_Offset(SliderElement,orientObj),
	"horizontal" },
    { NULL, 0, 0, NULL }
};

static void SliderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SliderElement *slider = elementRecord;
    int orient, length, thickness, borderWidth;

    Ttk_GetOrientFromObj(NULL, slider->orientObj, &orient);
    Tk_GetPixelsFromObj(NULL, tkwin, slider->borderWidthObj, &borderWidth);
    Tk_GetPixelsFromObj(NULL, tkwin, slider->lengthObj, &length);
    Tk_GetPixelsFromObj(NULL, tkwin, slider->thicknessObj, &thickness);

    switch (orient) {
	case TTK_ORIENT_VERTICAL:
	    *widthPtr = thickness + (borderWidth *2);
	    *heightPtr = *widthPtr/2;
	    break;

	case TTK_ORIENT_HORIZONTAL:
	    *heightPtr = thickness + (borderWidth *2);
	    *widthPtr = *heightPtr/2;
	    break;
    }
}

static void SliderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    SliderElement *slider = elementRecord;
    Tk_3DBorder border = Tk_Get3DBorderFromObj(tkwin, slider->borderObj);
    XColor *borderColor = Tk_GetColorFromObj(tkwin, slider->borderColorObj);
    int relief = TK_RELIEF_RAISED, borderWidth = 2;

    Tk_GetPixelsFromObj(NULL, tkwin, slider->borderWidthObj, &borderWidth);
    Tk_GetReliefFromObj(NULL, slider->reliefObj, &relief);

    Tk_Fill3DRectangle(tkwin, d, border,
	b.x, b.y, b.width, b.height,
	borderWidth, TK_RELIEF_FLAT);
    DrawBorder(tkwin, d, border, borderColor, b, borderWidth, relief);
}

static Ttk_ElementSpec SliderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(SliderElement),
    SliderElementOptions,
    SliderElementSize,
    SliderElementDraw
};

/*------------------------------------------------------------------------
 * +++ Tree indicator element.
 */

#define TTK_STATE_OPEN TTK_STATE_USER1		/* XREF: treeview.c */
#define TTK_STATE_LEAF TTK_STATE_USER2

typedef struct {
    Tcl_Obj *colorObj;
    Tcl_Obj *marginObj;
    Tcl_Obj *diameterObj;
} TreeitemIndicator;

static Ttk_ElementOptionSpec TreeitemIndicatorOptions[] = {
    { "-foreground", TK_OPTION_COLOR,
	Tk_Offset(TreeitemIndicator,colorObj), DEFAULT_FOREGROUND },
    { "-diameter", TK_OPTION_PIXELS,
	Tk_Offset(TreeitemIndicator,diameterObj), "9" },
    { "-indicatormargins", TK_OPTION_STRING,
	Tk_Offset(TreeitemIndicator,marginObj), "2 2 4 2" },
    { NULL, 0, 0, NULL }
};

static void TreeitemIndicatorSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    TreeitemIndicator *indicator = elementRecord;
    int diameter = 0;
    Ttk_Padding margins;

    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginObj, &margins);
    Tk_GetPixelsFromObj(NULL, tkwin, indicator->diameterObj, &diameter);
    *widthPtr = diameter + Ttk_PaddingWidth(margins);
    *heightPtr = diameter + Ttk_PaddingHeight(margins);
}

static void TreeitemIndicatorDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    TreeitemIndicator *indicator = elementRecord;
    XColor *color = Tk_GetColorFromObj(tkwin, indicator->colorObj);
    GC gc = Tk_GCForColor(color, d);
    Ttk_Padding padding = Ttk_UniformPadding(0);
    int w = WIN32_XDRAWLINE_HACK;
    int cx, cy;

    if (state & TTK_STATE_LEAF) {
	/* don't draw anything ... */
	return;
    }

    Ttk_GetPaddingFromObj(NULL,tkwin,indicator->marginObj,&padding);
    b = Ttk_PadBox(b, padding);

    XDrawRectangle(Tk_Display(tkwin), d, gc,
	    b.x, b.y, b.width - 1, b.height - 1);

    cx = b.x + (b.width - 1) / 2;
    cy = b.y + (b.height - 1) / 2;
    XDrawLine(Tk_Display(tkwin), d, gc, b.x+2, cy, b.x+b.width-3+w, cy);

    if (!(state & TTK_STATE_OPEN)) {
	/* turn '-' into a '+' */
	XDrawLine(Tk_Display(tkwin), d, gc, cx, b.y+2, cx, b.y+b.height-3+w);
    }
}

static Ttk_ElementSpec TreeitemIndicatorElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(TreeitemIndicator),
    TreeitemIndicatorOptions,
    TreeitemIndicatorSize,
    TreeitemIndicatorDraw
};

/*------------------------------------------------------------------------
 * TtkAltTheme_Init --
 * 	Install alternate theme.
 */
MODULE_SCOPE int TtkAltTheme_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme =  Ttk_CreateTheme(interp, "alt", NULL);

    if (!theme) {
	return TCL_ERROR;
    }

    Ttk_RegisterElement(interp, theme, "border", &BorderElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "Checkbutton.indicator",
	    &IndicatorElementSpec, &checkbutton_spec);
    Ttk_RegisterElement(interp, theme, "Radiobutton.indicator",
	    &IndicatorElementSpec, &radiobutton_spec);
    Ttk_RegisterElement(interp, theme, "Menubutton.indicator",
	    &MenubuttonArrowElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "field", &FieldElementSpec, NULL);

    Ttk_RegisterElement(interp, theme, "trough", &TroughElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "thumb", &ThumbElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "slider", &SliderElementSpec, NULL);

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

    Ttk_RegisterElement(interp, theme, "arrow",
	    &ArrowElementSpec, &ArrowElements[0]);

    Ttk_RegisterElement(interp, theme, "Treeitem.indicator",
	    &TreeitemIndicatorElementSpec, 0);

    Tcl_PkgProvide(interp, "ttk::theme::alt", TTK_VERSION);

    return TCL_OK;
}

/*EOF*/
