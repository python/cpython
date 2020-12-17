/* square.c - Copyright (C) 2004 Pat Thoyts <patthoyts@users.sourceforge.net>
 *
 * Minimal sample ttk widget.
 */

#include <tk.h>
#include "ttkTheme.h"
#include "ttkWidget.h"

#if defined(TTK_SQUARE_WIDGET) || 1

#ifndef DEFAULT_BORDERWIDTH
#define DEFAULT_BORDERWIDTH "2"
#endif

/*
 * First, we setup the widget record. The Ttk package provides a structure
 * that contains standard widget data so it is only necessary to define
 * a structure that holds the data required for our widget. We do this by
 * defining a widget part and then specifying the widget record as the
 * concatenation of the two structures.
 */

typedef struct
{
    Tcl_Obj *widthObj;
    Tcl_Obj *heightObj;
    Tcl_Obj *reliefObj;
    Tcl_Obj *borderWidthObj;
    Tcl_Obj *foregroundObj;
    Tcl_Obj *paddingObj;
    Tcl_Obj *anchorObj;
} SquarePart;

typedef struct
{
    WidgetCore core;
    SquarePart square;
} Square;

/*
 * Widget options.
 *
 * This structure is the same as the option specification structure used
 * for Tk widgets. For each option we provide the type, name and options
 * database name and class name and the position in the structure and
 * default values. At the bottom we bring in the standard widget option
 * defined for all widgets.
 */

static Tk_OptionSpec SquareOptionSpecs[] =
{
    {TK_OPTION_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
     DEFAULT_BORDERWIDTH, Tk_Offset(Square,square.borderWidthObj), -1,
     0,0,GEOMETRY_CHANGED },
    {TK_OPTION_BORDER, "-foreground", "foreground", "Foreground",
     DEFAULT_BACKGROUND, Tk_Offset(Square,square.foregroundObj),
     -1, 0, 0, 0},
    
    {TK_OPTION_PIXELS, "-width", "width", "Width",
     "50", Tk_Offset(Square,square.widthObj), -1, 0, 0,
     GEOMETRY_CHANGED},
    {TK_OPTION_PIXELS, "-height", "height", "Height",
     "50", Tk_Offset(Square,square.heightObj), -1, 0, 0,
     GEOMETRY_CHANGED},
    
    {TK_OPTION_STRING, "-padding", "padding", "Pad", NULL,
     Tk_Offset(Square,square.paddingObj), -1, 
     TK_OPTION_NULL_OK,0,GEOMETRY_CHANGED },
    
    {TK_OPTION_RELIEF, "-relief", "relief", "Relief",
     NULL, Tk_Offset(Square,square.reliefObj), -1, TK_OPTION_NULL_OK, 0, 0},
    
    {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor",
     NULL, Tk_Offset(Square,square.anchorObj), -1, TK_OPTION_NULL_OK, 0, 0},
    
    WIDGET_TAKEFOCUS_TRUE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

/*
 * Almost all of the widget functionality is handled by the default Ttk
 * widget code and the contained element. The one thing that we must handle
 * is the -anchor option which positions the square element within the parcel
 * of space available for the widget.
 * To do this we must find out the layout preferences for the square
 * element and adjust its position within our region.
 *
 * Note that if we do not have a "square" elememt then just the default
 * layout will be done. So if someone places a label element into the
 * widget layout it will still be handled but the -anchor option will be
 * passed onto the label element instead of handled here.
 */

static void
SquareDoLayout(void *clientData)
{
    WidgetCore *corePtr = (WidgetCore *)clientData;
    Ttk_Box winBox;
    Ttk_Element squareNode;

    squareNode = Ttk_FindElement(corePtr->layout, "square");
    winBox = Ttk_WinBox(corePtr->tkwin);
    Ttk_PlaceLayout(corePtr->layout, corePtr->state, winBox);

    /*
     * Adjust the position of the square element within the widget according
     * to the -anchor option.
     */

    if (squareNode) {
	Square *squarePtr = clientData;
	Tk_Anchor anchor = TK_ANCHOR_CENTER;
	Ttk_Box b;

	b = Ttk_ElementParcel(squareNode);
	if (squarePtr->square.anchorObj != NULL)
	    Tk_GetAnchorFromObj(NULL, squarePtr->square.anchorObj, &anchor);
	b = Ttk_AnchorBox(winBox, b.width, b.height, anchor);

	Ttk_PlaceElement(corePtr->layout, squareNode, b);
    }
}

/*
 * Widget commands. A widget is impelemented as an ensemble and the
 * subcommands are listed here. Ttk provides default implementations
 * that are sufficient for our needs.
 */

static const Ttk_Ensemble SquareCommands[] = {
    { "configure",	TtkWidgetConfigureCommand,0 },
    { "cget",		TtkWidgetCgetCommand,0 },
    { "identify",	TtkWidgetIdentifyCommand,0 },
    { "instate",	TtkWidgetInstateCommand,0 },
    { "state",  	TtkWidgetStateCommand,0 },
    { 0,0,0 }
};

/*
 * The Widget specification structure holds all the implementation 
 * information about this widget and this is what must be registered
 * with Tk in the package initialization code (see bottom).
 */

static WidgetSpec SquareWidgetSpec =
{
    "TSquare",			/* className */
    sizeof(Square),		/* recordSize */
    SquareOptionSpecs,		/* optionSpecs */
    SquareCommands,		/* subcommands */
    TtkNullInitialize,		/* initializeProc */
    TtkNullCleanup,		/* cleanupProc */
    TtkCoreConfigure,		/* configureProc */
    TtkNullPostConfigure,		/* postConfigureProc */
    TtkWidgetGetLayout,		/* getLayoutProc */
    TtkWidgetSize, 		/* sizeProc */
    SquareDoLayout,		/* layoutProc */
    TtkWidgetDisplay		/* displayProc */
};

/* ---------------------------------------------------------------------- 
 * Square element
 *
 * In this section we demonstrate what is required to create a new themed
 * element.
 */

typedef struct
{
    Tcl_Obj *borderObj;
    Tcl_Obj *foregroundObj;
    Tcl_Obj *borderWidthObj;
    Tcl_Obj *reliefObj;
    Tcl_Obj *widthObj;
    Tcl_Obj *heightObj;
} SquareElement;

static Ttk_ElementOptionSpec SquareElementOptions[] = 
{
    { "-background", TK_OPTION_BORDER, Tk_Offset(SquareElement,borderObj),
    	DEFAULT_BACKGROUND },
    { "-foreground", TK_OPTION_BORDER, Tk_Offset(SquareElement,foregroundObj),
    	DEFAULT_BACKGROUND },
    { "-borderwidth", TK_OPTION_PIXELS, Tk_Offset(SquareElement,borderWidthObj),
    	DEFAULT_BORDERWIDTH },
    { "-relief", TK_OPTION_RELIEF, Tk_Offset(SquareElement,reliefObj),
    	"raised" },
    { "-width",  TK_OPTION_PIXELS, Tk_Offset(SquareElement,widthObj), "20"},
    { "-height", TK_OPTION_PIXELS, Tk_Offset(SquareElement,heightObj), "20"},
    { NULL, 0, 0, NULL }
};

/*
 * The element geometry function is called when the layout code wishes to
 * find out how big this element wants to be. We must return our preferred
 * size and padding information
 */

static void SquareElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SquareElement *square = elementRecord;
    int borderWidth = 0;

    Tcl_GetIntFromObj(NULL, square->borderWidthObj, &borderWidth);
    *paddingPtr = Ttk_UniformPadding((short)borderWidth);
    Tk_GetPixelsFromObj(NULL, tkwin, square->widthObj, widthPtr);
    Tk_GetPixelsFromObj(NULL, tkwin, square->heightObj, heightPtr);
}

/*
 * Draw the element in the box provided.
 */

static void SquareElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    SquareElement *square = elementRecord;
    Tk_3DBorder foreground = NULL;
    int borderWidth = 1, relief = TK_RELIEF_FLAT;

    foreground = Tk_Get3DBorderFromObj(tkwin, square->foregroundObj);
    Tcl_GetIntFromObj(NULL, square->borderWidthObj, &borderWidth);
    Tk_GetReliefFromObj(NULL, square->reliefObj, &relief);

    Tk_Fill3DRectangle(tkwin, d, foreground,
	b.x, b.y, b.width, b.height, borderWidth, relief);
}

static Ttk_ElementSpec SquareElementSpec =
{
    TK_STYLE_VERSION_2,
    sizeof(SquareElement),
    SquareElementOptions,
    SquareElementSize,
    SquareElementDraw
};

/* ----------------------------------------------------------------------
 *
 * Layout section.
 *
 * Every widget class needs a layout style that specifies which elements
 * are part of the widget and how they should be placed. The element layout
 * engine is similar to the Tk pack geometry manager. Read the documentation
 * for the details. In this example we just need to have the square element
 * that has been defined for this widget placed on a background. We will
 * also need some padding to keep it away from the edges. 
 */

TTK_BEGIN_LAYOUT(SquareLayout)
     TTK_NODE("Square.background", TTK_FILL_BOTH)
     TTK_GROUP("Square.padding", TTK_FILL_BOTH,
	 TTK_NODE("Square.square", 0))
TTK_END_LAYOUT

/* ---------------------------------------------------------------------- 
 *
 * Widget initialization.
 *
 * This file defines a new element and a new widget. We need to register
 * the element with the themes that will need it. In this case we will 
 * register with the default theme that is the root of the theme inheritance
 * tree. This means all themes will find this element.
 * We then need to register the widget class style. This is the layout
 * specification. If a different theme requires an alternative layout, we
 * could register that here. For instance, in some themes the scrollbars have
 * one uparrow, in other themes there are two uparrow elements.
 * Finally we register the widget itself. This step creates a tcl command so
 * that we can actually create an instance of this class. The widget is
 * linked to a particular style by the widget class name. This is important
 * to realise as the programmer may change the classname when creating a
 * new instance. If this is done, a new layout will need to be created (which
 * can be done at script level). Some widgets may require particular elements
 * to be present but we try to avoid this where possible. In this widget's C
 * code, no reference is made to any particular elements. The programmer is
 * free to specify a new style using completely different elements.
 */

/* public */ MODULE_SCOPE int
TtkSquareWidget_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_GetDefaultTheme(interp);

    /* register the new elements for this theme engine */
    Ttk_RegisterElement(interp, theme, "square", &SquareElementSpec, NULL);
    
    /* register the layout for this theme */
    Ttk_RegisterLayout(theme, "TSquare", SquareLayout);
    
    /* register the widget */
    RegisterWidget(interp, "ttk::square", &SquareWidgetSpec);

    return TCL_OK;
}

#endif /* TTK_SQUARE_WIDGET */

