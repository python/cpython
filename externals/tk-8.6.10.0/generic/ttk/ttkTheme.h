/*
 * Copyright (c) 2003 Joe English.  Freely redistributable.
 *
 * Declarations for Tk theme engine.
 */

#ifndef _TTKTHEME
#define _TTKTHEME

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MODULE_SCOPE
#   ifdef __cplusplus
#	define MODULE_SCOPE extern "C"
#   else
#	define MODULE_SCOPE extern
#   endif
#endif

#define TTKAPI MODULE_SCOPE

/* Ttk syncs to the Tk version & patchlevel */
#define TTK_VERSION    TK_VERSION
#define TTK_PATCH_LEVEL TK_PATCH_LEVEL

/*------------------------------------------------------------------------
 * +++ Defaults for element option specifications.
 */
#define DEFAULT_FONT 		"TkDefaultFont"
#ifdef MAC_OSX_TK
#define DEFAULT_BACKGROUND	"systemTextBackgroundColor"
#define DEFAULT_FOREGROUND	"systemTextColor"
#else
#define DEFAULT_BACKGROUND	"#d9d9d9"
#define DEFAULT_FOREGROUND	"black"
#endif
/*------------------------------------------------------------------------
 * +++ Widget states.
 * 	Keep in sync with stateNames[] in tkstate.c.
 */

typedef unsigned int Ttk_State;

#define TTK_STATE_ACTIVE	(1<<0)
#define TTK_STATE_DISABLED	(1<<1)
#define TTK_STATE_FOCUS		(1<<2)
#define TTK_STATE_PRESSED	(1<<3)
#define TTK_STATE_SELECTED	(1<<4)
#define TTK_STATE_BACKGROUND	(1<<5)
#define TTK_STATE_ALTERNATE	(1<<6)
#define TTK_STATE_INVALID	(1<<7)
#define TTK_STATE_READONLY 	(1<<8)
#define TTK_STATE_HOVER		(1<<9)
#define TTK_STATE_USER6		(1<<10)
#define TTK_STATE_USER5		(1<<11)
#define TTK_STATE_USER4		(1<<12)
#define TTK_STATE_USER3		(1<<13)
#define TTK_STATE_USER2		(1<<14)
#define TTK_STATE_USER1		(1<<15)

/* Maintenance note: if you get all the way to "USER1",
 * see tkstate.c
 */
typedef struct
{
    unsigned int onbits;	/* bits to turn on */
    unsigned int offbits;	/* bits to turn off */
} Ttk_StateSpec;

#define Ttk_StateMatches(state, spec) \
    (((state) & ((spec)->onbits|(spec)->offbits)) == (spec)->onbits)

#define Ttk_ModifyState(state, spec) \
    (((state) & ~(spec)->offbits) | (spec)->onbits)

TTKAPI int Ttk_GetStateSpecFromObj(Tcl_Interp *, Tcl_Obj *, Ttk_StateSpec *);
TTKAPI Tcl_Obj *Ttk_NewStateSpecObj(unsigned int onbits,unsigned int offbits);

/*------------------------------------------------------------------------
 * +++ State maps and state tables.
 */
typedef Tcl_Obj *Ttk_StateMap;

TTKAPI Ttk_StateMap Ttk_GetStateMapFromObj(Tcl_Interp *, Tcl_Obj *);
TTKAPI Tcl_Obj *Ttk_StateMapLookup(Tcl_Interp*, Ttk_StateMap, Ttk_State);

/*
 * Table for looking up an integer index based on widget state:
 */
typedef struct
{
    int index;			/* Value to return if this entry matches */
    unsigned int onBits;	/* Bits which must be set */
    unsigned int offBits;	/* Bits which must be cleared */
} Ttk_StateTable;

TTKAPI int Ttk_StateTableLookup(Ttk_StateTable map[], Ttk_State);

/*------------------------------------------------------------------------
 * +++ Padding.
 * 	Used to represent internal padding and borders.
 */
typedef struct
{
    short left;
    short top;
    short right;
    short bottom;
} Ttk_Padding;

TTKAPI int Ttk_GetPaddingFromObj(Tcl_Interp*,Tk_Window,Tcl_Obj*,Ttk_Padding*);
TTKAPI int Ttk_GetBorderFromObj(Tcl_Interp*,Tcl_Obj*,Ttk_Padding*);

TTKAPI Ttk_Padding Ttk_MakePadding(short l, short t, short r, short b);
TTKAPI Ttk_Padding Ttk_UniformPadding(short borderWidth);
TTKAPI Ttk_Padding Ttk_AddPadding(Ttk_Padding, Ttk_Padding);
TTKAPI Ttk_Padding Ttk_RelievePadding(Ttk_Padding, int relief, int n);

#define Ttk_PaddingWidth(p) ((p).left + (p).right)
#define Ttk_PaddingHeight(p) ((p).top + (p).bottom)

#define Ttk_SetMargins(tkwin, pad) \
    Tk_SetInternalBorderEx(tkwin, pad.left, pad.right, pad.top, pad.bottom)

/*------------------------------------------------------------------------
 * +++ Boxes.
 * 	Used to represent rectangular regions
 */
typedef struct 	/* Hey, this is an XRectangle! */
{
    int x;
    int y;
    int width;
    int height;
} Ttk_Box;

TTKAPI Ttk_Box Ttk_MakeBox(int x, int y, int width, int height);
TTKAPI int Ttk_BoxContains(Ttk_Box, int x, int y);

#define Ttk_WinBox(tkwin) Ttk_MakeBox(0,0,Tk_Width(tkwin),Tk_Height(tkwin))

/*------------------------------------------------------------------------
 * +++ Layout utilities.
 */
typedef enum {
    TTK_SIDE_LEFT, TTK_SIDE_TOP, TTK_SIDE_RIGHT, TTK_SIDE_BOTTOM
} Ttk_Side;

typedef unsigned int Ttk_Sticky;

/*
 * -sticky bits for Ttk_StickBox:
 */
#define TTK_STICK_W	(0x1)
#define TTK_STICK_E	(0x2)
#define TTK_STICK_N	(0x4)
#define TTK_STICK_S	(0x8)

/*
 * Aliases and useful combinations:
 */
#define TTK_FILL_X	(0x3)	/* -sticky ew */
#define TTK_FILL_Y	(0xC)	/* -sticky ns */
#define TTK_FILL_BOTH	(0xF)	/* -sticky nswe */

TTKAPI int Ttk_GetStickyFromObj(Tcl_Interp *, Tcl_Obj *, Ttk_Sticky *);
TTKAPI Tcl_Obj *Ttk_NewStickyObj(Ttk_Sticky);

/*
 * Extra bits for position specifications (combine -side and -sticky)
 */

typedef unsigned int Ttk_PositionSpec;	/* See below */

#define TTK_PACK_LEFT	(0x10)	/* pack at left of current parcel */
#define TTK_PACK_RIGHT	(0x20)	/* pack at right of current parcel */
#define TTK_PACK_TOP	(0x40)	/* pack at top of current parcel */
#define TTK_PACK_BOTTOM	(0x80)	/* pack at bottom of current parcel */
#define TTK_EXPAND	(0x100)	/* use entire parcel */
#define TTK_BORDER	(0x200)	/* draw this element after children */
#define TTK_UNIT	(0x400)	/* treat descendants as a part of element */

/*
 * Extra bits for layout specifications
 */
#define _TTK_CHILDREN	(0x1000)/* for LayoutSpecs -- children follow */
#define _TTK_LAYOUT_END	(0x2000)/* for LayoutSpecs -- end of child list */
#define _TTK_LAYOUT	(0x4000)/* for LayoutSpec tables -- define layout */

#define _TTK_MASK_STICK (0x0F)	/* See Ttk_UnparseLayout() */
#define _TTK_MASK_PACK	(0xF0)	/* See Ttk_UnparseLayout(), packStrings */

TTKAPI Ttk_Box Ttk_PackBox(Ttk_Box *cavity, int w, int h, Ttk_Side side);
TTKAPI Ttk_Box Ttk_StickBox(Ttk_Box parcel, int w, int h, Ttk_Sticky sticky);
TTKAPI Ttk_Box Ttk_AnchorBox(Ttk_Box parcel, int w, int h, Tk_Anchor anchor);
TTKAPI Ttk_Box Ttk_PadBox(Ttk_Box b, Ttk_Padding p);
TTKAPI Ttk_Box Ttk_ExpandBox(Ttk_Box b, Ttk_Padding p);
TTKAPI Ttk_Box Ttk_PlaceBox(Ttk_Box *cavity, int w,int h, Ttk_Side,Ttk_Sticky);
TTKAPI Ttk_Box Ttk_PositionBox(Ttk_Box *cavity, int w, int h, Ttk_PositionSpec);

/*------------------------------------------------------------------------
 * +++ Themes.
 */
MODULE_SCOPE void Ttk_StylePkgInit(Tcl_Interp *);

typedef struct Ttk_Theme_ *Ttk_Theme;
typedef struct Ttk_ElementClass_ Ttk_ElementClass;
typedef struct Ttk_Layout_ *Ttk_Layout;
typedef struct Ttk_LayoutNode_ *Ttk_Element;
typedef struct Ttk_Style_ *Ttk_Style;

TTKAPI Ttk_Theme Ttk_GetTheme(Tcl_Interp *interp, const char *name);
TTKAPI Ttk_Theme Ttk_GetDefaultTheme(Tcl_Interp *interp);
TTKAPI Ttk_Theme Ttk_GetCurrentTheme(Tcl_Interp *interp);

TTKAPI Ttk_Theme Ttk_CreateTheme(
    Tcl_Interp *interp, const char *name, Ttk_Theme parent);

typedef int (Ttk_ThemeEnabledProc)(Ttk_Theme theme, void *clientData);
MODULE_SCOPE void Ttk_SetThemeEnabledProc(Ttk_Theme, Ttk_ThemeEnabledProc, void *);

MODULE_SCOPE int Ttk_UseTheme(Tcl_Interp *, Ttk_Theme);

typedef void (Ttk_CleanupProc)(void *clientData);
TTKAPI void Ttk_RegisterCleanup(
    Tcl_Interp *interp, void *deleteData, Ttk_CleanupProc *cleanupProc);

/*------------------------------------------------------------------------
 * +++ Elements.
 */

enum TTKStyleVersion2 { TK_STYLE_VERSION_2 = 2 };

typedef void (Ttk_ElementSizeProc)(void *clientData, void *elementRecord,
        Tk_Window tkwin, int *widthPtr, int *heightPtr, Ttk_Padding*);
typedef void (Ttk_ElementDrawProc)(void *clientData, void *elementRecord,
        Tk_Window tkwin, Drawable d, Ttk_Box b, Ttk_State state);

typedef struct Ttk_ElementOptionSpec
{
    const char *optionName;		/* Command-line name of the widget option */
    Tk_OptionType type; 	/* Accepted option types */
    int offset;			/* Offset of Tcl_Obj* field in element record */
    const char *defaultValue;		/* Default value to used if resource missing */
} Ttk_ElementOptionSpec;

#define TK_OPTION_ANY TK_OPTION_STRING

typedef struct Ttk_ElementSpec {
    enum TTKStyleVersion2 version;	/* Version of the style support. */
    size_t elementSize;			/* Size of element record */
    Ttk_ElementOptionSpec *options;	/* List of options, NULL-terminated */
    Ttk_ElementSizeProc *size;		/* Compute min size and padding */
    Ttk_ElementDrawProc *draw;  	/* Draw the element */
} Ttk_ElementSpec;

TTKAPI Ttk_ElementClass *Ttk_RegisterElement(
	Tcl_Interp *interp, Ttk_Theme theme, const char *elementName,
	Ttk_ElementSpec *, void *clientData);

typedef int (*Ttk_ElementFactory)
	(Tcl_Interp *, void *clientData,
	 Ttk_Theme, const char *elementName, int objc, Tcl_Obj *const objv[]);

TTKAPI int Ttk_RegisterElementFactory(
	Tcl_Interp *, const char *name, Ttk_ElementFactory, void *clientData);

/*
 * Null element implementation:
 * has no geometry or layout; may be used as a stub or placeholder.
 */

typedef struct {
    Tcl_Obj	*unused;
} NullElement;

MODULE_SCOPE void TtkNullElementSize
	(void *, void *, Tk_Window, int *, int *, Ttk_Padding *);
MODULE_SCOPE void TtkNullElementDraw
	(void *, void *, Tk_Window, Drawable, Ttk_Box, Ttk_State);
MODULE_SCOPE Ttk_ElementOptionSpec TtkNullElementOptions[];
MODULE_SCOPE Ttk_ElementSpec ttkNullElementSpec;

/*------------------------------------------------------------------------
 * +++ Layout templates.
 */
typedef struct {
    const char *	elementName;
    unsigned		opcode;
} TTKLayoutInstruction, *Ttk_LayoutSpec;

#define TTK_BEGIN_LAYOUT_TABLE(name) \
				static TTKLayoutInstruction name[] = {
#define TTK_LAYOUT(name, content) \
				{ name, _TTK_CHILDREN|_TTK_LAYOUT }, \
				content \
				{ 0, _TTK_LAYOUT_END },
#define TTK_GROUP(name, flags, children) \
				{ name, flags | _TTK_CHILDREN }, \
				children \
				{ 0, _TTK_LAYOUT_END },
#define TTK_NODE(name, flags)	{ name, flags },
#define TTK_END_LAYOUT_TABLE	{ 0, _TTK_LAYOUT | _TTK_LAYOUT_END } };

#define TTK_BEGIN_LAYOUT(name)	static TTKLayoutInstruction name[] = {
#define TTK_END_LAYOUT 		{ 0, _TTK_LAYOUT_END } };

TTKAPI void Ttk_RegisterLayout(
    Ttk_Theme theme, const char *className, Ttk_LayoutSpec layoutSpec);

TTKAPI void Ttk_RegisterLayouts(
    Ttk_Theme theme, Ttk_LayoutSpec layoutTable);

/*------------------------------------------------------------------------
 * +++ Layout instances.
 */

MODULE_SCOPE Ttk_Layout Ttk_CreateLayout(
    Tcl_Interp *, Ttk_Theme, const char *name,
    void *recordPtr, Tk_OptionTable, Tk_Window tkwin);

MODULE_SCOPE Ttk_Layout Ttk_CreateSublayout(
    Tcl_Interp *, Ttk_Theme, Ttk_Layout, const char *name, Tk_OptionTable);

MODULE_SCOPE void Ttk_FreeLayout(Ttk_Layout);

MODULE_SCOPE void Ttk_LayoutSize(Ttk_Layout,Ttk_State,int *widthPtr,int *heightPtr);
MODULE_SCOPE void Ttk_PlaceLayout(Ttk_Layout, Ttk_State, Ttk_Box);
MODULE_SCOPE void Ttk_DrawLayout(Ttk_Layout, Ttk_State, Drawable);

MODULE_SCOPE void Ttk_RebindSublayout(Ttk_Layout, void *recordPtr);

MODULE_SCOPE Ttk_Element Ttk_IdentifyElement(Ttk_Layout, int x, int y);
MODULE_SCOPE Ttk_Element Ttk_FindElement(Ttk_Layout, const char *nodeName);

MODULE_SCOPE const char *Ttk_ElementName(Ttk_Element);
MODULE_SCOPE Ttk_Box Ttk_ElementParcel(Ttk_Element);

MODULE_SCOPE Ttk_Box Ttk_ClientRegion(Ttk_Layout, const char *elementName);

MODULE_SCOPE Ttk_Box Ttk_LayoutNodeInternalParcel(Ttk_Layout,Ttk_Element);
MODULE_SCOPE Ttk_Padding Ttk_LayoutNodeInternalPadding(Ttk_Layout,Ttk_Element);
MODULE_SCOPE void Ttk_LayoutNodeReqSize(Ttk_Layout, Ttk_Element, int *w, int *h);

MODULE_SCOPE void Ttk_PlaceElement(Ttk_Layout, Ttk_Element, Ttk_Box);
MODULE_SCOPE void Ttk_ChangeElementState(Ttk_Element,unsigned set,unsigned clr);

MODULE_SCOPE Tcl_Obj *Ttk_QueryOption(Ttk_Layout, const char *, Ttk_State);

TTKAPI Ttk_Style Ttk_LayoutStyle(Ttk_Layout);
TTKAPI Tcl_Obj *Ttk_StyleDefault(Ttk_Style, const char *optionName);
TTKAPI Tcl_Obj *Ttk_StyleMap(Ttk_Style, const char *optionName, Ttk_State);

/*------------------------------------------------------------------------
 * +++ Resource cache.
 * 	See resource.c for explanation.
 */

typedef struct Ttk_ResourceCache_ *Ttk_ResourceCache;
MODULE_SCOPE Ttk_ResourceCache Ttk_CreateResourceCache(Tcl_Interp *);
MODULE_SCOPE void Ttk_FreeResourceCache(Ttk_ResourceCache);

MODULE_SCOPE Ttk_ResourceCache Ttk_GetResourceCache(Tcl_Interp*);
MODULE_SCOPE Tcl_Obj *Ttk_UseFont(Ttk_ResourceCache, Tk_Window, Tcl_Obj *);
MODULE_SCOPE Tcl_Obj *Ttk_UseColor(Ttk_ResourceCache, Tk_Window, Tcl_Obj *);
MODULE_SCOPE Tcl_Obj *Ttk_UseBorder(Ttk_ResourceCache, Tk_Window, Tcl_Obj *);
MODULE_SCOPE Tk_Image Ttk_UseImage(Ttk_ResourceCache, Tk_Window, Tcl_Obj *);

MODULE_SCOPE void Ttk_RegisterNamedColor(Ttk_ResourceCache, const char *, XColor *);

/*------------------------------------------------------------------------
 * +++ Image specifications.
 */

typedef struct TtkImageSpec Ttk_ImageSpec;
TTKAPI Ttk_ImageSpec *TtkGetImageSpec(Tcl_Interp *, Tk_Window, Tcl_Obj *);
TTKAPI Ttk_ImageSpec *TtkGetImageSpecEx(Tcl_Interp *, Tk_Window, Tcl_Obj *,
					Tk_ImageChangedProc *, ClientData);
TTKAPI void TtkFreeImageSpec(Ttk_ImageSpec *);
TTKAPI Tk_Image TtkSelectImage(Ttk_ImageSpec *, Ttk_State);

/*------------------------------------------------------------------------
 * +++ Miscellaneous enumerations.
 * 	Other stuff that element implementations need to know about.
 */
typedef enum 			/* -default option values */
{
    TTK_BUTTON_DEFAULT_NORMAL,	/* widget defaultable */
    TTK_BUTTON_DEFAULT_ACTIVE,	/* currently the default widget */
    TTK_BUTTON_DEFAULT_DISABLED	/* not defaultable */
} Ttk_ButtonDefaultState;

TTKAPI int Ttk_GetButtonDefaultStateFromObj(Tcl_Interp *, Tcl_Obj *, int *);

typedef enum 			/* -compound option values */
{
    TTK_COMPOUND_NONE,  	/* image if specified, otherwise text */
    TTK_COMPOUND_TEXT,  	/* text only */
    TTK_COMPOUND_IMAGE,  	/* image only */
    TTK_COMPOUND_CENTER,	/* text overlays image */
    TTK_COMPOUND_TOP,   	/* image above text */
    TTK_COMPOUND_BOTTOM,	/* image below text */
    TTK_COMPOUND_LEFT,   	/* image to left of text */
    TTK_COMPOUND_RIGHT  	/* image to right of text */
} Ttk_Compound;

TTKAPI int Ttk_GetCompoundFromObj(Tcl_Interp *, Tcl_Obj *, int *);

typedef enum { 		/* -orient option values */
    TTK_ORIENT_HORIZONTAL,
    TTK_ORIENT_VERTICAL
} Ttk_Orient;

/*------------------------------------------------------------------------
 * +++ Utilities.
 */

typedef struct TtkEnsemble {
    const char *name;			/* subcommand name */
    Tcl_ObjCmdProc *command; 		/* subcommand implementation, OR: */
    const struct TtkEnsemble *ensemble;	/* subcommand ensemble */
} Ttk_Ensemble;

MODULE_SCOPE int Ttk_InvokeEnsemble(	/* Run an ensemble command */
    const Ttk_Ensemble *commands, int cmdIndex,
    void *clientData, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[]);

MODULE_SCOPE int TtkEnumerateHashTable(Tcl_Interp *, Tcl_HashTable *);

/*------------------------------------------------------------------------
 * +++ Stub table declarations.
 */

#include "ttkDecls.h"

/*
 * Drawing utilities for theme code:
 * (@@@ find a better home for this)
 */
typedef enum { ARROW_UP, ARROW_DOWN, ARROW_LEFT, ARROW_RIGHT } ArrowDirection;
MODULE_SCOPE void TtkArrowSize(int h, ArrowDirection, int *widthPtr, int *heightPtr);
MODULE_SCOPE void TtkDrawArrow(Display *, Drawable, GC, Ttk_Box, ArrowDirection);
MODULE_SCOPE void TtkFillArrow(Display *, Drawable, GC, Ttk_Box, ArrowDirection);

#ifdef __cplusplus
}
#endif
#endif /* _TTKTHEME */
