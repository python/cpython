
/*	$Id: tixHList.h,v 1.2 2004/03/28 02:44:56 hobbs Exp $	*/

/*
 * tixHList.h --
 *
 *	Defines the data structures and functions used by the tixHList
 *	widget.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TIX_HLIST_H_
#define _TIX_HLIST_H_

#ifndef  _TIX_INT_H_
#include <tixInt.h>
#endif

#define HLTYPE_COLUMN 1
#define HLTYPE_HEADER 2
#define HLTYPE_ENTRY  3

/* This is used to indetify what object has caused a DItemSizeChange
 * All data structs for objects that manage DItems must have these two
 * members as the beginning of the struct.
 */
typedef struct HLItemTypeInfo {
    int type;
    char * self;
} HLItemTypeInfo;

typedef struct HListColumn {
    /* generic type info section */
    int type;
    char * self;
    struct _HListElement * chPtr;

    /* other data */
    Tix_DItem * iPtr;
    int width;
} HListColumn;

typedef struct HListHeader {
    /* generic type info section */
    int type;
    char * self;

    struct HListStruct * wPtr;
    /* other data */
    Tix_DItem * iPtr;
    int width;

    Tk_3DBorder background;	/* Used for drawing the 3d border. */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    int borderWidth;
} HListHeader;

/*----------------------------------------------------------------------
 *  A HListElement structure contain the information about each element
 * inside the HList.
 *
 */
typedef struct _HListElement {
    /* generic type info section */
    int type;
    char * self;

    /* other data */
    struct HListStruct   * wPtr;
    struct _HListElement * parent;
    struct _HListElement * prev;
    struct _HListElement * next;
    struct _HListElement * childHead;
    struct _HListElement * childTail;

    int numSelectedChild;	/* number of childs that has selection(s) in 
				 * them (either this child is selected or some
				 * of its descendants are selected */
    int numCreatedChild;	/* this var gets increment by one each
				 * time a child is created */
    char * pathName;		/* Full pathname of this element */
    char * name;		/* Name of this element */
    int height;			/* Height of this element, including padding
				 * and selBorderWidth;
				 */
    int allHeight;		/* Height of all descendants and self */
    Tk_Uid state;		/* State of Tab's for display purposes:
				 * normal or disabled. */
    char * data;		/* user data field */

    /* bottom-middle position of the bitmap/image branch (offset from
     * the top-left corner of the item)
     */
    int branchX;		
    int branchY;

    /*  offset of the left-middle position of the icon */
    int iconX;
    int iconY;
    /*----------------------------------*/
    /* Things to display in the element */
    /*----------------------------------*/
    HListColumn * col;		/* the multi-column display items */
    HListColumn  _oneCol;	/* If we have only one column, then this
				 * space is used (pointed to by column).
				 * This will save one Malloc */
    int indent;
    Tix_DItem * indicator;	/* indicator: little triangle on Mac */

    /*----------------------------------*/
    /* Flags			        */
    /*----------------------------------*/
    Tix_DItemInfo * diTypePtr;

    unsigned int selected : 1;
    unsigned int hidden   : 1;
    unsigned int dirty    : 1;	/* If it is dirty then its geometry needs
				 * be recalculated */
} Tix_HListElement, HListElement;

/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */
typedef struct HListStruct {
    Tix_DispData dispData;
    Tcl_Command widgetCmd;	/* Token for button's widget command. */

    /*
     * Information used when displaying widget:
     */
    char *command;		/* Command prefix to use when invoking
				 * scrolling commands.  NULL means don't
				 * invoke commands.  Malloc'ed. */
    int width, height;		/* For app programmer to request size */

    /*
     * Information used when displaying widget:
     */

    /* Border and general drawing */
    int borderWidth;		/* Width of 3-D borders. */
    int selBorderWidth;		/* Width of 3-D borders for selected items */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    int indent;			/* How much should the children be indented
				 * (to the right)?, in pixels */
    Tk_3DBorder border;		/* Used for drawing the 3d border. */
    Tk_3DBorder selectBorder;	/* Used for selected background. */
    XColor *normalFg;		/* Normal foreground for text. */
    XColor *normalBg;		/* Normal bachground for  text. */
    XColor *selectFg;		/* Color for drawing selected text. */
    TixFont font;		/* The default font used in the DItems. */
    GC backgroundGC;		/* GC for drawing background. */
    GC normalGC;		/* GC for drawing text in normal mode. */
    GC selectGC;		/* GC for drawing selected background. */
    GC anchorGC;		/* GC for drawing dotted anchor highlight. */
    GC dropSiteGC;		/* GC for drawing dotted anchor highlight. */

    Cursor cursor;		/* Current cursor for window, or None. */

    int topPixel;		/* Vertical offset */
    int leftPixel;		/* Horizontal offset */
    int bottomPixel;
    int wideSelect;		/* BOOL: if 1, use a wide selection: the 
				 * selection background color covers the whole
				 * widget. If 0, only the "significant" part
				 * of a list entry is highlighted */
    int selectWidth;		/* Width of the selection: takes effect only
				 * if wideSelect == 1 */
    /* For highlights */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    GC highlightGC;		/* For drawing traversal highlight. */

    /* default pad and gap values */
    int gap, padX, padY;
    char * separator;

    Tk_Uid selectMode;		/* Selection style: single, browse, multiple,
				 * or extended.  This value isn't used in C
				 * code, but the Tcl bindings use it. */
    int drawBranch;		/* Whether to draw the "branch" lines from
				 * parent entry to children */
    Tcl_HashTable childTable;	/* Hash table to translate child names
				 * into (HListElement *) */
    HListElement * root;	/* Mother of all elements */
    HListElement * anchor;	/* The current anchor item */
    HListElement * dragSite;	/* The current drag site */
    HListElement * dropSite;	/* The current drop site */

    char *yScrollCmd;		/* Command prefix for communicating with
				 * vertical scrollbar.  NULL means no command
				 * to issue.  Malloc'ed. */
    char *xScrollCmd;		/* Command prefix for communicating with
				 * horizontal scrollbar. NULL means no command
				 * to issue.  Malloc'ed. */
    char *sizeCmd;		/* The command to call when the size of
				 * the listbox changes. E.g., when the user
				 * add/deletes elements. Useful for
				 * auto-scrollbar geometry managers */
    char *browseCmd;		/* The command to call when the selection
				 * changes. */
    char *indicatorCmd;		/* The command to call when the user touches
				 * the indicator. */
    char *dragCmd;		/* The command to call when info about a 
				 * drag source is needed */
    char *dropCmd;		/* The command to call when action at a drop
				 * side needs to be performed */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */

    Tix_LinkList mappedWindows; /* Those windows that are are mapped by this
				 * widget*/
    int serial;			/* this number is incremented before each time
				 * the widget is redisplayed */

    int numColumns;		/* number of columns in the tixHList widget,
				 * cannot be changed after the widget's
				 * creation */

    int totalSize[2];

    HListColumn * reqSize;	/* Requested column sizes by the user:
				   take precedence */
    HListColumn * actualSize;	/* Actual column sizes, calculated using
				 * the sizes of the ditems */

    HListHeader ** headers;	/* Stores all the headers for a HList widget */
    int useHeader;		/* whether headers should be used */
    int headerHeight;		/* required height of the header */

    Tix_DItemInfo * diTypePtr;	/* Default item type */
    Tix_StyleTemplate stTmpl;

    int useIndicator;		/* should indicators be displayed */
    int scrollUnit[2];

    Tk_Window headerWin;	/* subwindow, used to draw the headers */
    char * elmToSee;		/* name of element to "see" the next time
				 * this widget is redrawn */
    unsigned redrawing : 1;
    unsigned redrawingFrame : 1;
    unsigned resizing  : 1;
    unsigned hasFocus  : 1;
    unsigned allDirty  : 1;
    unsigned initialized : 1;
    unsigned headerDirty : 1;
    unsigned needToRaise : 1;	/* The header subwindow needs to be raised
				 * if we add a new window item into the
				 * HList widget (either in the list or
				 * in the header */
} HList;

#define TIX_X 0
#define TIX_Y 1
#define UNINITIALIZED -1

typedef HList   WidgetRecord;
typedef HList * WidgetPtr;

EXTERN HListColumn * 	Tix_HLAllocColumn _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement * chPtr));
EXTERN void		Tix_HLCancelResizeWhenIdle _ANSI_ARGS_((
			    WidgetPtr wPtr));
EXTERN void		Tix_HLComputeGeometry _ANSI_ARGS_((
			    ClientData clientData));
EXTERN HListElement * 	Tix_HLFindElement _ANSI_ARGS_((Tcl_Interp *interp,
			    WidgetPtr wPtr, CONST84 char * pathName));
EXTERN void 		Tix_HLFreeMappedWindow _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement * chPtr));
EXTERN int	 	Tix_HLElementTopOffset _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement *chPtr));
EXTERN int	 	Tix_HLElementLeftOffset _ANSI_ARGS_((
			    WidgetPtr wPtr, HListElement *chPtr));
EXTERN int 		Tix_HLItemInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    WidgetPtr wPtr, int argc, CONST84 char** argv));
EXTERN int		Tix_HLHeader _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, CONST84 char **argv));
EXTERN int		Tix_HLCreateHeaders _ANSI_ARGS_((
			    Tcl_Interp *interp, WidgetPtr wPtr));
EXTERN void		Tix_HLFreeHeaders _ANSI_ARGS_((
			    Tcl_Interp *interp, WidgetPtr wPtr));
EXTERN void		Tix_HLDrawHeader _ANSI_ARGS_((
			    WidgetPtr wPtr, Pixmap pixmap, GC gc,
			    int hdrX, int hdrY, int hdrW, int hdrH,
			    int xOffset));
EXTERN void		Tix_HLComputeHeaderGeometry _ANSI_ARGS_((
			    WidgetPtr wPtr));

EXTERN void  		Tix_HLMarkElementDirty _ANSI_ARGS_((WidgetPtr wPtr,
			    HListElement *chPtr));
EXTERN void		Tix_HLResizeWhenIdle _ANSI_ARGS_((WidgetPtr wPtr));
EXTERN void		Tix_HLResizeNow _ANSI_ARGS_((WidgetPtr wPtr));
EXTERN void		Tix_HLComputeGeometry _ANSI_ARGS_((
			    ClientData clientData));
EXTERN void		Tix_HLCancelResizeWhenIdle _ANSI_ARGS_((
			    WidgetPtr wPtr));


/* in tixHLCol.c */
EXTERN TIX_DECLARE_SUBCMD(Tix_HLColumn);
EXTERN TIX_DECLARE_SUBCMD(Tix_HLItem);

/* in tixHLInd.c */
EXTERN TIX_DECLARE_SUBCMD(Tix_HLIndicator);


#endif /*_TIX_HLIST_H_ */

