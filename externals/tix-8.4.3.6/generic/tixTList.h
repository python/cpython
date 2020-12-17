
/*	$Id: tixTList.h,v 1.2 2000/12/24 07:06:22 ioilam Exp $	*/

/*
 * tixTList.h --
 *
 *	This header file defines the data structures used by the tixTList
 *	widget.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TIX_TLIST_H_
#define _TIX_TLIST_H_

#define TIX_X 0
#define TIX_Y 1

typedef struct ListEntry {
    struct ListEntry * next;
    Tix_DItem * iPtr;
    Tk_Uid state;
    int size[2];
    unsigned int selected : 1;
} ListEntry;

typedef struct ListRow {
    ListEntry * chPtr;
    int size[2];
    int numEnt;
} ListRow;

/*
 * A data structure of the following type is kept for each
 * widget managed by this file:
 */
typedef struct ListStruct {
    Tix_DispData dispData;

    Tcl_Command widgetCmd;	/* Token for button's widget command. */

    /*
     * Information used when displaying widget:
     */
    int width, height;		/* For app programmer to request size */

    /*
     * Information used when displaying widget:
     */

    /* Border and general drawing */
    int borderWidth;		/* Width of 3-D borders. */
    int selBorderWidth;		/* Width of 3-D borders for selected items */
    int relief;			/* Indicates whether window as a whole is
				 * raised, sunken, or flat. */
    Tk_3DBorder border;		/* Used for drawing the 3d border. */
    Tk_3DBorder selectBorder;	/* Used for selected background. */
    XColor *normalFg;		/* Normal foreground for text. */
    XColor *normalBg;		/* Normal background for  text. */
    XColor *selectFg;		/* Color for drawing selected text. */

       /* GC and stuff */
    GC backgroundGC;		/* GC for drawing background. */
    GC selectGC;		/* GC for drawing selected background. */
    GC anchorGC;		/* GC for drawing dotted anchor highlight
                                 * around a selected item */
    GC anchorGC2;		/* GC for drawing dotted anchor highlight
                                 * around an unselected item */
    TixFont font;		/* Default font used by the DItems. */

    /* Text drawing */
    Cursor cursor;		/* Current cursor for window, or None. */

    /* For highlights */
    int highlightWidth;		/* Width in pixels of highlight to draw
				 * around widget when it has the focus.
				 * <= 0 means don't draw a highlight. */
    XColor *highlightColorPtr;	/* Color for drawing traversal highlight. */
    GC highlightGC;		/* For drawing traversal highlight. */

    /* default pad and gap values */
    int padX, padY;

    Tk_Uid selectMode;		/* Selection style: single, browse, multiple,
				 * or extended.  This value isn't used in C
				 * code, but the Tcl bindings use it. */
    Tk_Uid state;		/* State can only be normal or disabled. */
    Tix_LinkList entList;

    int numRowAllocd;
    int numRow;
    ListRow * rows;

    ListEntry * seeElemPtr;	/* The current item to "see" */
    ListEntry * anchor;		/* The current anchor item */
    ListEntry * active;		/* The current active item */
    ListEntry * dropSite;	/* The current drop site */
    ListEntry * dragSite;	/* The current drop site */

    /*
     * Commands 
     */
    char *command;		/* The command when user double-clicks */
    char *browseCmd;		/* The command to call when the selection
				 * changes. */
    char *sizeCmd;		/* The command to call when the size of
				 * the listbox changes. E.g., when the user
				 * add/deletes elements. Useful for
				 * auto-scrollbar geometry managers */

    /* These options control how the items are arranged on the list */
    Tk_Uid orientUid;		/* Can be "vertical" or "horizontal" */
    int packMode[2];		/* is row and column packed */
    int numMajor[2];		/* num of rows and columns */
    int itemSize[2];		/* horizontal and vertical size of items, -1
				 * means natural size */

    /* Info for laying out */
    int maxSize[2];		/* max size of all elements in X and Y, (they
				 * do not need to be the same element, may be
				 * invalid according to mode */
    char *takeFocus;		/* Value of -takefocus option;  not used in
				 * the C code, but used by keyboard traversal
				 * scripts.  Malloc'ed, but may be NULL. */

    int serial;			/* this number is incremented before each time
				 * the widget is redisplayed */

    Tix_DItemInfo * diTypePtr;	/* Default item type */
    Tix_IntScrollInfo scrollInfo[2];
    unsigned int redrawing : 1;
    unsigned int resizing  : 1;
    unsigned int hasFocus  : 1;
    unsigned int isVertical : 1;
} TixTListWidget;

typedef TixTListWidget   WidgetRecord;
typedef TixTListWidget * WidgetPtr;

#endif /* _TIX_TLIST_H_ */
