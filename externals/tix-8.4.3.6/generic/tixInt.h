/*
 * tixInt.h --
 *
 *	Defines internal data types and functions used by the Tix library.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixInt.h,v 1.7 2008/02/28 04:29:17 hobbs Exp $
 */

#ifndef _TIX_INT_H_
#define _TIX_INT_H_

#ifndef _TIX_H_
#include <tix.h>
#endif

#ifndef _TIX_PORT_H_
#include <tixPort.h>
#endif

/*----------------------------------------------------------------------
 *
 *		Tix Display Item Types
 *
 *----------------------------------------------------------------------
 */

#define TIX_DITEM_NONE			0
#define TIX_DITEM_TEXT			1
#define TIX_DITEM_IMAGETEXT		2
#define TIX_DITEM_WINDOW		3
#define TIX_DITEM_IMAGE			4

/*
 * The following 12 values can be OR'ed to passed as the flags
 * parameter to Tix_DItemDisplay().
 */

#define TIX_DITEM_NORMAL_BG		(0x1 <<	 0)
#define TIX_DITEM_ACTIVE_BG		(0x1 <<	 1)
#define TIX_DITEM_SELECTED_BG		(0x1 <<	 2)
#define TIX_DITEM_DISABLED_BG		(0x1 <<	 3)
#define TIX_DITEM_NORMAL_FG		(0x1 <<	 4)
#define TIX_DITEM_ACTIVE_FG		(0x1 <<	 5)
#define TIX_DITEM_SELECTED_FG		(0x1 <<	 6)
#define TIX_DITEM_DISABLED_FG		(0x1 <<	 7)
#define TIX_DITEM_FONT			(0x1 <<	 8)
#define TIX_DITEM_PADX			(0x1 <<	 9)
#define TIX_DITEM_PADY			(0x1 << 10)
#define TIX_DITEM_ANCHOR                (0x1 << 11)

#define TIX_DITEM_OTHER_BG \
    (TIX_DITEM_ACTIVE_BG|TIX_DITEM_SELECTED_BG|TIX_DITEM_DISABLED_BG)

#define TIX_DITEM_ALL_BG \
    (TIX_DITEM_NORMAL_BG|TIX_DITEM_OTHER_BG)

#define TIX_DONT_CALL_CONFIG		TK_CONFIG_USER_BIT

/*
 * These values are used ONLY for indexing the color array in
 * Tix_StyleTemplate
 */

#define TIX_DITEM_NORMAL		0
#define TIX_DITEM_ACTIVE		1
#define TIX_DITEM_SELECTED		2
#define TIX_DITEM_DISABLED		3

/*
 * Flags for MultiInfo
 */
#define TIX_CONFIG_INFO			1
#define TIX_CONFIG_VALUE		2

typedef union  Tix_DItem		Tix_DItem;
typedef union  Tix_DItemStyle		Tix_DItemStyle;
typedef struct Tix_DItemInfo		Tix_DItemInfo;
typedef struct Tix_DispData		Tix_DispData;
typedef struct Tix_StyleTemplate	Tix_StyleTemplate;

typedef void		Tix_DItemCalculateSizeProc(Tix_DItem * iPtr);
typedef char *		Tix_DItemComponentProc(Tix_DItem * iPtr, int x, int y);
typedef int		Tix_DItemConfigureProc(Tix_DItem * iPtr,
	int argc, CONST84 char **argv, int flags);
typedef Tix_DItem *	Tix_DItemCreateProc(Tix_DispData * ddPtr,
	Tix_DItemInfo * diTypePtr);
typedef void		Tix_DItemDisplayProc(Drawable drawable,
	Tix_DItem * iPtr,
	int x, int y, int width, int height,
	int xOffset, int yOffset, int flag);
typedef void		Tix_DItemFreeProc(Tix_DItem * diPtr);
typedef void		Tix_DItemSizeChangedProc(Tix_DItem * iPtr);

typedef void		Tix_DItemStyleChangedProc(Tix_DItem * iPtr);
typedef void		Tix_DItemLostStyleProc(Tix_DItem * iPtr);
typedef int		Tix_DItemStyleConfigureProc(Tix_DItemStyle* style,
	int argc, CONST84 char **argv, int flags);
typedef Tix_DItemStyle*	Tix_DItemStyleCreateProc(Tcl_Interp * interp,
	Tk_Window tkwin,
	Tix_DItemInfo * diTypePtr, char * name);
typedef void		Tix_DItemStyleFreeProc(Tix_DItemStyle* style);
typedef void		Tix_DItemStyleSetTemplateProc(Tix_DItemStyle* style,
	Tix_StyleTemplate * tmplPtr);

/*
 * These are debugging routines
 */

typedef int		Tix_DItemRefCountProc();
typedef int		Tix_DItemStyleRefCountProc();

/*----------------------------------------------------------------------
 * Tix_DItemInfo --
 *
 *	This structure is used to register a new display item (call
 *	Tix_AddDItemType).
 *----------------------------------------------------------------------
 */
struct Tix_DItemInfo {
    char * name;
    int type;

    /*
     * These procedures communicate with the items
     */
    Tix_DItemCreateProc * createProc;
    Tix_DItemConfigureProc * configureProc;
    Tix_DItemCalculateSizeProc * calculateSizeProc;
    Tix_DItemComponentProc * componentProc;
    Tix_DItemDisplayProc * displayProc;
    Tix_DItemFreeProc * freeProc;
    Tix_DItemStyleChangedProc *styleChangedProc;
    Tix_DItemLostStyleProc * lostStyleProc;

    /*
     * These procedures communicate with the styles
     */
    Tix_DItemStyleCreateProc * styleCreateProc;
    Tix_DItemStyleConfigureProc * styleConfigureProc;
    Tix_DItemStyleFreeProc * styleFreeProc;
    Tix_DItemStyleSetTemplateProc * styleSetTemplateProc;

    Tk_ConfigSpec * itemConfigSpecs;
    Tk_ConfigSpec * styleConfigSpecs;
    struct Tix_DItemInfo * next;
};

/*----------------------------------------------------------------------
 * Tix_DispData --
 *
 *	Information needed by the display types to display the item in
 *	an X drawable.
 *----------------------------------------------------------------------
 */
struct Tix_DispData {
    Display * display;
    Tcl_Interp * interp;
    Tk_Window tkwin;
    Tix_DItemSizeChangedProc * sizeChangedProc;
};

/*----------------------------------------------------------------------
 * Tix_StyleTemplate --
 *
 *	A StyleTemplate is used to set the values of the default styles
 *	associated with a widget
 *----------------------------------------------------------------------
 */
struct Tix_StyleTemplate {
    int flags;			/* determines which field is valid */

    struct {
	XColor * bg;
	XColor * fg;
    } colors[4];		/* colors for the four basic modes*/

    int pad[2];
#if 0
    /* %bordercolor not used */
    XColor * borderColor;
    Tix_Relief relief;
    int borderWidth;
#endif
    TixFont font;
};

/*----------------------------------------------------------------------
 *
 *
 *			Display Item Types
 *
 *
 *----------------------------------------------------------------------
 */

/*
 *  Display Styles
 */
typedef struct TixBaseStyle		TixBaseStyle;
typedef struct TixImageTextStyle	TixImageTextStyle;
typedef struct TixImageStyle		TixImageStyle;
typedef struct TixTextStyle		TixTextStyle;
typedef struct TixWindowStyle		TixWindowStyle;

typedef struct TixBaseItem		TixBaseItem;
typedef struct TixColorStyle		TixColorStyle;
typedef struct TixImageTextItem		TixImageTextItem;
typedef struct TixImageItem		TixImageItem;
typedef struct TixTextItem		TixTextItem;
typedef struct TixWindowItem		TixWindowItem;

/*----------------------------------------------------------------------
 * TixBaseItem --
 *
 *	This is the abstract base class for all display items. All
 *	display items should have the data members defined in the
 *	BaseItem structure
 *----------------------------------------------------------------------
 */
#define ITEM_COMMON_MEMBERS \
    Tix_DItemInfo * diTypePtr; \
    Tix_DispData * ddPtr; \
    ClientData clientData; \
    int size[2];		/* Size of this element */ \
    int selX, selY, selW, selH  /* Location of the selection highlight */

struct TixBaseItem {
    ITEM_COMMON_MEMBERS;
    TixBaseStyle * stylePtr;
};

/*----------------------------------------------------------------------
 * TixBaseStyle --
 *
 *	This is the abstract base class for all display styles. All
 *	display items should have the data members defined in the
 *	BaseStyle structure.  The common members are initialized by
 *	tixDiStyle.c
 *
 *----------------------------------------------------------------------
 */

#define STYLE_COMMON_MEMBERS \
    Tcl_Command styleCmd;	/* Token for style's command. */ \
    Tcl_HashTable items;	/* Ditems affected by this style */ \
    int refCount;		/* Number of ditems affected by this style */\
    int flags;			/* Various attributes */ \
    Tcl_Interp *interp;		/* Interpreter associated with style. */ \
    Tk_Window tkwin;		/* Window associated with this style */ \
    Tix_DItemInfo * diTypePtr; \
    Tk_Anchor anchor;		/* Anchor information */ \
    char * name;		/* Name of this style */ \
    int pad[2];			/* paddings */ \
    \
    struct { \
	XColor * bg; \
	XColor * fg; \
	GC foreGC;   \
	GC backGC;   \
        GC anchorGC; \
    } colors[4]			/* colors and GC's for the four basic modes*/


#define STYLE_COLOR_MEMBERS     /* Backwards-cimpatibility */

struct TixBaseStyle {
    STYLE_COMMON_MEMBERS;
};

#define TIX_STYLE_DELETED 1
#define TIX_STYLE_DEFAULT 2

/*
 * Abstract type for all styles that have a color element
 */
struct TixColorStyle {
    STYLE_COMMON_MEMBERS;
};

/*----------------------------------------------------------------------
 * ImageTextItem --
 *
 *	Display an image together with a text string
 *----------------------------------------------------------------------
 */
struct TixImageTextItem {
    ITEM_COMMON_MEMBERS;

    TixImageTextStyle *stylePtr;
	/*-------------------------*/
	/*	 Bitmap		   */
	/*-------------------------*/
    Pixmap bitmap;
    int bitmapW, bitmapH;	/* Size of bitmap */

	/*-------------------------*/
	/*	 Image		   */
	/*-------------------------*/
    char *imageString;		/* Name of image to display (malloc'ed), or
				 * NULL.  If non-NULL, bitmap, text, and
				 * textVarName are ignored. */
    Tk_Image image;
    int imageW, imageH;		/* Size of image */

	/*-------------------------*/
	/*	 Text		  */
	/*-------------------------*/

    char * text;		/* Show descriptive text */
    size_t numChars;		/* Size of text */
    int textW, textH;
    int wrapLength;
    Tk_Justify justify;		/* Justification to use for multi-line text. */
    int underline;		/* Index of character to underline.  < 0 means
				 * don't underline anything. */

    int showImage, showText;
};

struct TixImageTextStyle {
    STYLE_COMMON_MEMBERS;
    int wrapLength;
    Tk_Justify justify;		/* Justification to use for multi-line text. */
    TixFont font;
    int gap;			/* Gap between text and image */
};

/*----------------------------------------------------------------------
 * ImageItem --
 *
 *	Displays an image
 *----------------------------------------------------------------------
 */
struct TixImageItem {
    ITEM_COMMON_MEMBERS;

    TixImageStyle *stylePtr;

	/*-------------------------*/
	/*	 Image		   */
	/*-------------------------*/
    char *imageString;		/* Name of image to display (malloc'ed), or
				 * NULL.  If non-NULL, bitmap, text, and
				 * textVarName are ignored. */
    Tk_Image image;
    int imageW, imageH;		/* Size of image */
};

struct TixImageStyle {
    STYLE_COMMON_MEMBERS;
};
/*----------------------------------------------------------------------
 * TextItem --
 *
 *	Displays a text string.
 *----------------------------------------------------------------------
 */
struct TixTextItem {
    ITEM_COMMON_MEMBERS;

    TixTextStyle *stylePtr;
	/*-------------------------*/
	/*	 Text		  */
	/*-------------------------*/

    char * text;		/* Show descriptive text */
    int numChars;		/* Size of text */
    int textW, textH;
    int underline;		/* Index of character to underline.  < 0 means
				 * don't underline anything. */
};

struct TixTextStyle {
    STYLE_COMMON_MEMBERS;
    int wrapLength;
    Tk_Justify justify;		/* Justification to use for multi-line text. */
    TixFont font;
};

/*----------------------------------------------------------------------
 * WindowItem --
 *
 *	Displays a window.
 *----------------------------------------------------------------------
 */
struct TixWindowItem {
    ITEM_COMMON_MEMBERS;
    TixWindowStyle *stylePtr;
    Tk_Window tkwin;
    struct TixWindowItem * next;
    int serial;
};

struct TixWindowStyle {
    STYLE_COMMON_MEMBERS;
};

/*----------------------------------------------------------------------
 * Tix_DItem and Tix_DItemStyle --
 *
 *	These unions just make it easy to address the internals of the
 *	structures of the display items and styles. If you create a new
 *	display item, you will need to do you type casting yourself.
 *----------------------------------------------------------------------
 */
union Tix_DItem {
    TixBaseItem		base;
    TixImageTextItem	imagetext;
    TixTextItem		text;
    TixWindowItem	window;
    TixImageItem	image;
};

union Tix_DItemStyle {
    TixBaseStyle	base;
    TixColorStyle	color;
    TixImageTextStyle	imagetext;
    TixTextStyle	text;
    TixWindowStyle	window;
    TixImageStyle	image;
};

#define Tix_DItemType(x)	((x)->base.diTypePtr->type)
#define Tix_DItemTypeName(x)	((x)->base.diTypePtr->name)
#define Tix_DItemWidth(x)	((x)->base.size[0])
#define Tix_DItemHeight(x)	((x)->base.size[1])
#define Tix_DItemConfigSpecs(x) ((x)->base.diTypePtr->itemConfigSpecs)
#define Tix_DItemPadX(x)	((x)->base.stylePtr->pad[0])
#define Tix_DItemPadY(x)	((x)->base.stylePtr->pad[1])

#define TIX_WIDTH  0
#define TIX_HEIGHT 1

typedef struct _TixpSubRegion TixpSubRegion;

/*----------------------------------------------------------------------
 * Tix_ArgumentList --
 * 
 *	This data structure is used to split command arguments for
 *	the display item types
 *----------------------------------------------------------------------
 */
#define FIXED_SIZE 4
typedef struct {
    int argc;
    CONST84 char **argv;
} Tix_Argument;

typedef struct {
    Tix_Argument * arg;
    int numLists;
    Tix_Argument preAlloc[FIXED_SIZE];
} Tix_ArgumentList;

/*----------------------------------------------------------------------
 * Tix_ScrollInfo --
 * 
 *	This data structure encapsulates all the necessary operations
 *	for scrolling widgets
 *----------------------------------------------------------------------
 */
#define TIX_SCROLL_INT		1
#define TIX_SCROLL_DOUBLE	2

/* abstract type */
typedef struct Tix_ScrollInfo {
    int type;		/* TIX_SCROLL_INT or TIX_SCROLL_DOUBLE */
    char * command;
} Tix_ScrollInfo;

typedef struct Tix_IntScrollInfo {
    int type;		/* TIX_SCROLL_INT */
    char * command;

    int total;		/* total size (width or height) of the widget*/
    int window;		/* visible size */
    int offset;		/* The top/left side of the scrolled widget */
    int unit;		/* How much should we scroll when the user
			 * press the arrow on a scrollbar? */

} Tix_IntScrollInfo;

typedef struct Tix_DoubleScrollInfo {
    int type;		/* TIX_SCROLL_DOUBLE */
    char * command;

    double total;	/* total size (width or height) of the widget*/
    double window;	/* visible size */
    double offset;	/* The top/left side of the scrolled widget */
    double unit;	/* How much should we scroll when the user
			 * press the arrow on a scrollbar? */
} Tix_DoubleScrollInfo;

/*----------------------------------------------------------------------
 *
 *		Global variables
 *
 * Should be used only in the Tix library. Some systems don't support
 * exporting of global variables from shared libraries.
 *
 *----------------------------------------------------------------------
 */
EXTERN Tk_Uid tixNormalUid;
EXTERN Tk_Uid tixDisabledUid;
EXTERN Tk_Uid tixCellUid;
EXTERN Tk_Uid tixRowUid;
EXTERN Tk_Uid tixColumnUid;

#define FLAG_READONLY	0
#define FLAG_STATIC	1
#define FLAG_FORCECALL	2

/*----------------------------------------------------------------------
 *
 *
 *		    MEGA-WIDGET CONFIG HANDLING
 *
 *
 *----------------------------------------------------------------------
 */
typedef struct _TixConfigSpec		TixConfigSpec;
typedef struct _TixConfigAlias		TixConfigAlias;
typedef struct _TixClassRecord		TixClassRecord;

struct _TixConfigSpec {
    unsigned int isAlias	: 1;
    unsigned int readOnly	: 1;
    unsigned int isStatic	: 1;
    unsigned int forceCall	: 1;

    char *argvName;
    char * defValue;

    char * dbName;		/* The additional parts of a */
    char * dbClass;		/* TixWidgetConfigSpec structure */

    char *verifyCmd;

    TixConfigSpec * realPtr;	/* valid only if this option is an alias */
};

/*
 * Controls the access of root widget and subwidget commands and options
 */
typedef struct _Tix_ExportSpec {
    Tix_LinkList exportCmds;
    Tix_LinkList restrictCmds;
    Tix_LinkList exportOpts;
    Tix_LinkList restrictOpts;
} Tix_ExportSpec;

typedef struct _Tix_SubWidgetSpec {
    struct _Tix_SubWidgetSpec * next;
    CONST84 char * name;
    Tix_ExportSpec export;
} Tix_SubWidgetSpec;

typedef struct _Tix_StringLink {
    struct _Tix_StringLink *next;
    CONST84 char * string;
} Tix_StringLink;

typedef struct _Tix_SubwidgetDef {
    struct _TixSubwidgetDef * next;
    CONST84 char * spec;
    CONST84 char * value;
} Tix_SubwidgetDef;

typedef struct _TixClassParseStruct {
    CONST84 char * alias;
    CONST84 char * ClassName;
    CONST84 char * configSpec;
    CONST84 char * def;
    CONST84 char * flag;
    CONST84 char * forceCall;
    CONST84 char * method;
    CONST84 char * readOnly;
    CONST84 char * isStatic;
    CONST84 char * superClass;
    CONST84 char * subWidget;
    CONST84 char * isVirtual;

    int	    optArgc;
    CONST84 char ** optArgv;
} TixClassParseStruct;

struct _TixClassRecord {
    TixClassRecord    * next;		/* Chains to the next class record in
					 * a superClass's unInitSubCls list */
    TixClassRecord    * superClass;	/* The superclass of this class. Is
					 * NULL if this class does not have
					 * a superclass. */
    unsigned int	isWidget;	/* TRUE iff this class is created by
					 * the "tixWidgetClass" command */
    char	      * className;	/* Instiantiation command */
    char	      * ClassName;	/* used in TK option database */

    int			nSpecs;
    TixConfigSpec    ** specs;
    int			nMethods;
    char	     ** methods;
    Tk_Window		mainWindow;	/* This variable is essentially
					 * a cached variable so that
					 * we can advoid calling
					 * Tk_MainWindow() */
    int			isVirtual;	/* is this a virtual base class
					 * (shouldn't be instantiated)*/
    TixClassParseStruct*parsePtr;	/* Information supplied by the
					 * tixClass or tixWidgetClass
					 * commands */
    Tix_LinkList	unInitSubCls;	/* The subclasses that have not been
					 * initialized. */
    int			initialized;	/* Is this class initialized? A class
					 * is not initialized if it has been
					 * defined but some its superclass
					 * is not initialized.
					 */
    Tix_LinkList	subWDefs;	/* the -defaults option */
#if USE_ACCESS_CONTROL
    Tix_LinkList	subWidgets;
    Tix_ExportSpec	exportSpec;	/* controls the export status
					 * of the commands and options
					 * of the root widget */
#endif
};

typedef struct _TixInterpState {
    char * result;
    char * errorInfo;
    char * errorCode;
} TixInterpState;

/*----------------------------------------------------------------------
 *
 *		Internal procedures
 *
 *----------------------------------------------------------------------
 */

EXTERN int		Tix_CallConfigMethod(
    Tcl_Interp *interp, TixClassRecord *cPtr,
    CONST84 char * widRec, TixConfigSpec *spec, CONST84 char * value);
EXTERN int		Tix_CallMethod(Tcl_Interp *interp,
	CONST84 char *context, CONST84 char *widRec, CONST84 char *method,
	int argc, CONST84 char **argv, int *foundPtr);
EXTERN int		Tix_ChangeOneOption(
    Tcl_Interp *interp, TixClassRecord *cPtr,
    CONST84 char * widRec, TixConfigSpec *spec, CONST84 char * value,
    int isDefault, int isInit);
EXTERN int		Tix_ChangeOptions(
			    Tcl_Interp *interp, TixClassRecord *cPtr,
			    CONST84 char * widRec, int argc, CONST84 char **argv);
EXTERN TixConfigSpec *	Tix_FindConfigSpecByName(
			    Tcl_Interp * interp,
			    TixClassRecord * cPtr, CONST84 char * name);
EXTERN CONST84 char  *	Tix_FindMethod(Tcl_Interp *interp,
			    CONST84 char *context, CONST84 char *method);
EXTERN char *		Tix_FindPublicMethod(
			    Tcl_Interp *interp, TixClassRecord * cPtr, 
			    CONST84 char * method);
EXTERN int		Tix_GetChars(Tcl_Interp *interp,
			    CONST84 char *string, double *doublePtr);
EXTERN CONST84 char  *	Tix_GetConfigSpecFullName(CONST84 char *clasRec,
			    CONST84 char *flag);
EXTERN CONST84 char *	Tix_GetContext(
			    Tcl_Interp * interp, CONST84 char * widRec);
EXTERN CONST84 char *	Tix_GetMethodFullName(CONST84 char *context,
			    CONST84 char *method);
EXTERN void		Tix_GetPublicMethods(Tcl_Interp *interp,
			    CONST84 char *widRec, int *numMethods,
			    char *** validMethods);
EXTERN int		Tix_GetWidgetOption(
			    Tcl_Interp *interp, Tk_Window tkwin,
			    CONST84 char *argvName, CONST84 char *dbName, CONST84 char *dbClass,
			    CONST84 char *defValue, int argc, CONST84 char **argv,
			    int type, char *ptr);
EXTERN int		Tix_GetVar(
			    Tcl_Interp *interp, TixClassRecord *cPtr,
			    CONST84 char * widRec, CONST84 char * flag);
EXTERN int		Tix_QueryAllOptions(
			    Tcl_Interp *interp, TixClassRecord * cPtr,
			    CONST84 char *widRec);
EXTERN int		Tix_QueryOneOption(
			    Tcl_Interp *interp, TixClassRecord *cPtr,
			    CONST84 char *widRec, CONST84 char *flag);
EXTERN int		Tix_SuperClass(Tcl_Interp *interp,
			    CONST84 char *widClass, CONST84 char ** superClass_ret);
EXTERN int		Tix_UnknownPublicMethodError(
			    Tcl_Interp *interp, TixClassRecord * cPtr,
			    CONST84 char * widRec, CONST84 char * method);
EXTERN int		Tix_ValueMissingError(Tcl_Interp *interp,
			    CONST84 char *spec);
EXTERN void		Tix_AddDItemType(
			    Tix_DItemInfo * diTypePtr);
EXTERN int		Tix_ConfigureInfo2(
			    Tcl_Interp *interp, Tk_Window tkwin,
			    CONST84 char *entRec, Tk_ConfigSpec *entConfigSpecs,
			    Tix_DItem * iPtr, CONST84 char *argvName, int flags);
EXTERN int		Tix_ConfigureValue2(Tcl_Interp *interp,
			    Tk_Window tkwin, CONST84 char * entRec,
			    Tk_ConfigSpec *entConfigSpecs, Tix_DItem * iPtr,
			    CONST84 char *argvName, int flags);
EXTERN void		Tix_DItemCalculateSize(
			    Tix_DItem * iPtr);
EXTERN char *		Tix_DItemComponent(Tix_DItem * diPtr,
			    int x, int y);
EXTERN int		Tix_DItemConfigure(
			    Tix_DItem * diPtr, int argc,
			    CONST84 char **argv, int flags);
EXTERN Tix_DItem *	Tix_DItemCreate(Tix_DispData * ddPtr,
			    CONST84 char * type);
EXTERN void		Tix_DItemDrawBackground(Drawable drawable,
                           TixpSubRegion *subRegPtr, Tix_DItem * iPtr,
                           int x, int y, int width, int height, 
                           int xOffset, int yOffset, int flags);
EXTERN void		Tix_DItemDisplay(
			    Drawable drawable, Tix_DItem * iPtr,
			    int x, int y, int width, int height,
                            int xOffset, int yOffset, int flag);
EXTERN int              Tix_DItemFillNormalBG(Drawable drawable,
                           TixpSubRegion *subRegPtr, Tix_DItem * iPtr,
                           int x, int y, int width, int height, 
                           int xOffset, int yOffset, int flags);
EXTERN void		Tix_DItemFree(
			    Tix_DItem * iPtr);
EXTERN void		TixDItemStyleChanged(
			    Tix_DItemInfo * diTypePtr,
			    Tix_DItemStyle * stylePtr);
EXTERN void             TixDItemStyleConfigureGCs(
                            Tix_DItemStyle *style);
EXTERN void		TixDItemStyleFree(Tix_DItem *iPtr, 
			    Tix_DItemStyle * stylePtr);
EXTERN void		TixDItemGetAnchor(Tk_Anchor anchor,
			    int x, int y, int cav_w, int cav_h,
			    int width, int height, int * x_ret, int * y_ret);
EXTERN void		Tix_FreeArgumentList(
			    Tix_ArgumentList *argListPtr);
EXTERN void		TixGetColorDItemGC(
			    Tix_DItem * iPtr, GC * backGC_ret,
			    GC * foreGC_ret, GC * anchorGC_ret, int flags);
EXTERN Tix_DItemStyle*	TixGetDefaultDItemStyle(
			    Tix_DispData * ddPtr, Tix_DItemInfo * diTypePtr,
			    Tix_DItem *iPtr, Tix_DItemStyle* oldStylePtr);
EXTERN Tix_DItemInfo *	Tix_GetDItemType(
			    Tcl_Interp * interp, CONST84 char *type);
EXTERN void		Tix_GetScrollFractions(
			    Tix_ScrollInfo * siPtr,
			    double * first_ret, double * last_ret);
EXTERN void		Tix_InitScrollInfo(
			    Tix_ScrollInfo * siPtr, int type);
EXTERN int		Tix_MultiConfigureInfo(
			    Tcl_Interp * interp,
			    Tk_Window tkwin, Tk_ConfigSpec **specsList,
			    int numLists, CONST84 char **widgRecList, CONST84 char *argvName,
			    int flags, int request);
EXTERN void		Tix_SetDefaultStyleTemplate(
			    Tk_Window tkwin, Tix_StyleTemplate * tmplPtr);
EXTERN int		Tix_SetScrollBarView(
			    Tcl_Interp *interp, Tix_ScrollInfo * siPtr,
			    int argc, CONST84 char **argv, int compat);
EXTERN void		Tix_SetWindowItemSerial(
			    Tix_LinkList * lPtr, Tix_DItem * iPtr,
			    int serial);
EXTERN int		Tix_SplitConfig(Tcl_Interp * interp,
			    Tk_Window tkwin, Tk_ConfigSpec  ** specsList,
			    int numLists, int argc, CONST84 char **argv,
			    Tix_ArgumentList * argListPtr);
EXTERN void		Tix_UnmapInvisibleWindowItems(
			    Tix_LinkList * lPtr, int serial);
EXTERN void		Tix_UpdateScrollBar(
			    Tcl_Interp *interp, Tix_ScrollInfo * siPtr);
EXTERN int		Tix_WidgetConfigure2(
			    Tcl_Interp *interp, Tk_Window tkwin, CONST84 char * entRec,
			    Tk_ConfigSpec *entConfigSpecs,
			    Tix_DItem * iPtr, int argc, CONST84 char **argv,
			    int flags, int forced, int * sizeChanged_ret);
EXTERN void		Tix_WindowItemListRemove(
			    Tix_LinkList * lPtr, Tix_DItem * iPtr);
/* 
 * Functions that should be used by Tix only. Functions prefixed by "Tix"
 * are generic functions that has one implementation for all platforms.
 * Functions prefixed with "Tixp" requires one implementation on each
 * platform.
 */

extern void             TixInitializeDisplayItems(void);
extern void		TixpDrawAnchorLines(Display *display,
			    Drawable drawable, GC gc, int x, int y,
			    int w, int h);
extern void		TixpDrawTmpLine(int x1, int y1,
			    int x2, int y2, Tk_Window tkwin);
extern void		TixpEndSubRegionDraw(Display *display,
			     Drawable drawable, GC gc,
			     TixpSubRegion * subRegPtr);
extern int		TixpSetWindowParent(Tcl_Interp * interp,
			    Tk_Window tkwin, Tk_Window newParent,
			    int parentId);
extern void		TixpStartSubRegionDraw(Display *display,
			     Drawable drawable, GC gc,
			     TixpSubRegion * subRegPtr, int origX,
			     int origY, int x, int y, int width, int height,
			     int needWidth, int needHeight);
extern void		TixpSubRegDisplayText(Display *display,
			    Drawable drawable, GC gc,
			    TixpSubRegion * subRegPtr,
			    TixFont font, CONST84 char *string,
			    int numChars, int x, int y, int length,
			    Tk_Justify justify, int underline);
extern void		TixpSubRegDrawBitmap(Display *display,
			    Drawable drawable, GC gc,
			    TixpSubRegion * subRegPtr, Pixmap bitmap,
			    int src_x, int src_y, int width, int height,
			    int dest_x, int dest_y, unsigned long plane);
extern void 		TixpSubRegDrawImage(
			    TixpSubRegion * subRegPtr, Tk_Image image,
			    int imageX, int imageY, int width, int height,
			    Drawable drawable, int drawableX, int drawableY);
extern void             TixpSubRegDrawAnchorLines(
                            Display *display, Drawable drawable,
                            GC gc, TixpSubRegion * subRegPtr,
                            int x, int y, int w, int h);
extern void		TixpSubRegFillRectangle(Display *display,
			    Drawable drawable, GC gc,
			    TixpSubRegion * subRegPtr, int x, int y,
			    int width, int height);
extern void             TixpSubRegSetClip(
                            Display *display, TixpSubRegion * subRegPtr,
                            GC gc);
extern void             TixpSubRegUnsetClip(
                            Display *display, TixpSubRegion * subRegPtr,
                            GC gc);
extern char *           tixStrDup( CONST char * s);
extern int 		TixMwmProtocolHandler(
			    ClientData clientData, XEvent *eventPtr);

/*
 * Image types implemented by Tix.
 */

extern Tk_ImageType tixPixmapImageType;
extern Tk_ImageType tixCompoundImageType;

/*
 * Display Items implemented in the Tix core.
 */

extern Tix_DItemInfo tix_ImageTextItemType;
extern Tix_DItemInfo tix_TextItemType;
extern Tix_DItemInfo tix_WindowItemType;
extern Tix_DItemInfo tix_ImageItemType;

#endif /* _TIX_INT_H_ */
