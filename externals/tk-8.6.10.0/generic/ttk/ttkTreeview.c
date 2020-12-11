/*
 * Copyright (c) 2004, Joe English
 *
 * ttk::treeview widget implementation.
 */

#include <string.h>
#include <stdio.h>
#include "tkInt.h"
#include "ttkTheme.h"
#include "ttkWidget.h"

#define DEF_TREE_ROWS		"10"
#define DEF_COLWIDTH		"200"
#define DEF_MINWIDTH		"20"

static const int DEFAULT_ROWHEIGHT 	= 20;
static const int DEFAULT_INDENT 	= 20;
static const int HALO   		= 4;	/* separator */

#define TTK_STATE_OPEN TTK_STATE_USER1
#define TTK_STATE_LEAF TTK_STATE_USER2

#define STATE_CHANGED	 	(0x100)	/* item state option changed */

/*------------------------------------------------------------------------
 * +++ Tree items.
 *
 * INVARIANTS:
 * 	item->children	==> item->children->parent == item
 *	item->next	==> item->next->parent == item->parent
 * 	item->next 	==> item->next->prev == item
 * 	item->prev 	==> item->prev->next == item
 */

typedef struct TreeItemRec TreeItem;
struct TreeItemRec {
    Tcl_HashEntry *entryPtr;	/* Back-pointer to hash table entry */
    TreeItem	*parent;	/* Parent item */
    TreeItem	*children;	/* Linked list of child items */
    TreeItem	*next;		/* Next sibling */
    TreeItem	*prev;		/* Previous sibling */

    /*
     * Options and instance data:
     */
    Ttk_State 	state;
    Tcl_Obj	*textObj;
    Tcl_Obj	*imageObj;
    Tcl_Obj	*valuesObj;
    Tcl_Obj	*openObj;
    Tcl_Obj	*tagsObj;

    /*
     * Derived resources:
     */
    Ttk_TagSet	tagset;
    Ttk_ImageSpec *imagespec;
};

#define ITEM_OPTION_TAGS_CHANGED	0x100
#define ITEM_OPTION_IMAGE_CHANGED	0x200

static Tk_OptionSpec ItemOptionSpecs[] = {
    {TK_OPTION_STRING, "-text", "text", "Text",
	"", Tk_Offset(TreeItem,textObj), -1,
	0,0,0 },
    {TK_OPTION_STRING, "-image", "image", "Image",
	NULL, Tk_Offset(TreeItem,imageObj), -1,
	TK_OPTION_NULL_OK,0,ITEM_OPTION_IMAGE_CHANGED },
    {TK_OPTION_STRING, "-values", "values", "Values",
	NULL, Tk_Offset(TreeItem,valuesObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_BOOLEAN, "-open", "open", "Open",
	"0", Tk_Offset(TreeItem,openObj), -1,
	0,0,0 },
    {TK_OPTION_STRING, "-tags", "tags", "Tags",
	NULL, Tk_Offset(TreeItem,tagsObj), -1,
	TK_OPTION_NULL_OK,0,ITEM_OPTION_TAGS_CHANGED },

    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0,0,0}
};

/* + NewItem --
 * 	Allocate a new, uninitialized, unlinked item
 */
static TreeItem *NewItem(void)
{
    TreeItem *item = ckalloc(sizeof(*item));

    item->entryPtr = 0;
    item->parent = item->children = item->next = item->prev = NULL;

    item->state = 0ul;
    item->textObj = NULL;
    item->imageObj = NULL;
    item->valuesObj = NULL;
    item->openObj = NULL;
    item->tagsObj = NULL;

    item->tagset = NULL;
    item->imagespec = NULL;

    return item;
}

/* + FreeItem --
 * 	Destroy an item
 */
static void FreeItem(TreeItem *item)
{
    if (item->textObj) { Tcl_DecrRefCount(item->textObj); }
    if (item->imageObj) { Tcl_DecrRefCount(item->imageObj); }
    if (item->valuesObj) { Tcl_DecrRefCount(item->valuesObj); }
    if (item->openObj) { Tcl_DecrRefCount(item->openObj); }
    if (item->tagsObj) { Tcl_DecrRefCount(item->tagsObj); }

    if (item->tagset)	{ Ttk_FreeTagSet(item->tagset); }
    if (item->imagespec) { TtkFreeImageSpec(item->imagespec); }

    ckfree(item);
}

static void FreeItemCB(void *clientData) { FreeItem(clientData); }

/* + DetachItem --
 * 	Unlink an item from the tree.
 */
static void DetachItem(TreeItem *item)
{
    if (item->parent && item->parent->children == item)
	item->parent->children = item->next;
    if (item->prev)
	item->prev->next = item->next;
    if (item->next)
	item->next->prev = item->prev;
    item->next = item->prev = item->parent = NULL;
}

/* + InsertItem --
 * 	Insert an item into the tree after the specified item.
 *
 * Preconditions:
 * 	+ item is currently detached
 * 	+ prev != NULL ==> prev->parent == parent.
 */
static void InsertItem(TreeItem *parent, TreeItem *prev, TreeItem *item)
{
    item->parent = parent;
    item->prev = prev;
    if (prev) {
	item->next = prev->next;
	prev->next = item;
    } else {
	item->next = parent->children;
	parent->children = item;
    }
    if (item->next) {
	item->next->prev = item;
    }
}

/* + NextPreorder --
 * 	Return the next item in preorder traversal order.
 */

static TreeItem *NextPreorder(TreeItem *item)
{
    if (item->children)
	return item->children;
    while (!item->next) {
	item = item->parent;
	if (!item)
	    return 0;
    }
    return item->next;
}

/*------------------------------------------------------------------------
 * +++ Display items and tag options.
 */

typedef struct {
    Tcl_Obj *textObj;		/* taken from item / data cell */
    Tcl_Obj *imageObj;		/* taken from item */
    Tcl_Obj *anchorObj;		/* from column <<NOTE-ANCHOR>> */
    Tcl_Obj *backgroundObj;	/* remainder from tag */
    Tcl_Obj *foregroundObj;
    Tcl_Obj *fontObj;
} DisplayItem;

static Tk_OptionSpec TagOptionSpecs[] = {
    {TK_OPTION_STRING, "-text", "text", "Text",
	NULL, Tk_Offset(DisplayItem,textObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_STRING, "-image", "image", "Image",
	NULL, Tk_Offset(DisplayItem,imageObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor",
	NULL, Tk_Offset(DisplayItem,anchorObj), -1,
	TK_OPTION_NULL_OK, 0, GEOMETRY_CHANGED},	/* <<NOTE-ANCHOR>> */
    {TK_OPTION_COLOR, "-background", "windowColor", "WindowColor",
	NULL, Tk_Offset(DisplayItem,backgroundObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_COLOR, "-foreground", "textColor", "TextColor",
	NULL, Tk_Offset(DisplayItem,foregroundObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_FONT, "-font", "font", "Font",
	NULL, Tk_Offset(DisplayItem,fontObj), -1,
	TK_OPTION_NULL_OK,0,GEOMETRY_CHANGED },

    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0,0,0}
};

/*------------------------------------------------------------------------
 * +++ Columns.
 *
 * There are separate option tables associated with the column record:
 * ColumnOptionSpecs is for configuring the column,
 * and HeadingOptionSpecs is for drawing headings.
 */
typedef struct {
    int 	width;		/* Column width, in pixels */
    int 	minWidth;	/* Minimum column width, in pixels */
    int 	stretch;	/* Should column stretch while resizing? */
    Tcl_Obj	*idObj;		/* Column identifier, from -columns option */

    Tcl_Obj	*anchorObj;	/* -anchor for cell data <<NOTE-ANCHOR>> */

    /* Column heading data:
     */
    Tcl_Obj 	*headingObj;		/* Heading label */
    Tcl_Obj	*headingImageObj;	/* Heading image */
    Tcl_Obj 	*headingAnchorObj;	/* -anchor for heading label */
    Tcl_Obj	*headingCommandObj;	/* Command to execute */
    Tcl_Obj 	*headingStateObj;	/* @@@ testing ... */
    Ttk_State	headingState;		/* ... */

    /* Temporary storage for cell data
     */
    Tcl_Obj 	*data;
} TreeColumn;

static void InitColumn(TreeColumn *column)
{
    column->width = 200;
    column->minWidth = 20;
    column->stretch = 1;
    column->idObj = 0;
    column->anchorObj = 0;

    column->headingState = 0;
    column->headingObj = 0;
    column->headingImageObj = 0;
    column->headingAnchorObj = 0;
    column->headingStateObj = 0;
    column->headingCommandObj = 0;

    column->data = 0;
}

static void FreeColumn(TreeColumn *column)
{
    if (column->idObj) { Tcl_DecrRefCount(column->idObj); }
    if (column->anchorObj) { Tcl_DecrRefCount(column->anchorObj); }

    if (column->headingObj) { Tcl_DecrRefCount(column->headingObj); }
    if (column->headingImageObj) { Tcl_DecrRefCount(column->headingImageObj); }
    if (column->headingAnchorObj) { Tcl_DecrRefCount(column->headingAnchorObj); }
    if (column->headingStateObj) { Tcl_DecrRefCount(column->headingStateObj); }
    if (column->headingCommandObj) { Tcl_DecrRefCount(column->headingCommandObj); }

    /* Don't touch column->data, it's scratch storage */
}

static Tk_OptionSpec ColumnOptionSpecs[] = {
    {TK_OPTION_INT, "-width", "width", "Width",
	DEF_COLWIDTH, -1, Tk_Offset(TreeColumn,width),
	0,0,GEOMETRY_CHANGED },
    {TK_OPTION_INT, "-minwidth", "minWidth", "MinWidth",
	DEF_MINWIDTH, -1, Tk_Offset(TreeColumn,minWidth),
	0,0,0 },
    {TK_OPTION_BOOLEAN, "-stretch", "stretch", "Stretch",
	"1", -1, Tk_Offset(TreeColumn,stretch),
	0,0,GEOMETRY_CHANGED },
    {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor",
	"w", Tk_Offset(TreeColumn,anchorObj), -1,	/* <<NOTE-ANCHOR>> */
	0,0,0 },
    {TK_OPTION_STRING, "-id", "id", "ID",
	NULL, Tk_Offset(TreeColumn,idObj), -1,
	TK_OPTION_NULL_OK,0,READONLY_OPTION },
    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0,0,0}
};

static Tk_OptionSpec HeadingOptionSpecs[] = {
    {TK_OPTION_STRING, "-text", "text", "Text",
	"", Tk_Offset(TreeColumn,headingObj), -1,
	0,0,0 },
    {TK_OPTION_STRING, "-image", "image", "Image",
	"", Tk_Offset(TreeColumn,headingImageObj), -1,
	0,0,0 },
    {TK_OPTION_ANCHOR, "-anchor", "anchor", "Anchor",
	"center", Tk_Offset(TreeColumn,headingAnchorObj), -1,
	0,0,0 },
    {TK_OPTION_STRING, "-command", "", "",
	"", Tk_Offset(TreeColumn,headingCommandObj), -1,
	TK_OPTION_NULL_OK,0,0 },
    {TK_OPTION_STRING, "state", "", "",
	"", Tk_Offset(TreeColumn,headingStateObj), -1,
	0,0,STATE_CHANGED },
    {TK_OPTION_END, 0,0,0, NULL, -1,-1, 0,0,0}
};

/*------------------------------------------------------------------------
 * +++ -show option:
 * TODO: Implement SHOW_BRANCHES.
 */

#define SHOW_TREE 	(0x1) 	/* Show tree column? */
#define SHOW_HEADINGS	(0x2)	/* Show heading row? */

#define DEFAULT_SHOW	"tree headings"

static const char *showStrings[] = {
    "tree", "headings", NULL
};

static int GetEnumSetFromObj(
    Tcl_Interp *interp,
    Tcl_Obj *objPtr,
    const char *table[],
    unsigned *resultPtr)
{
    unsigned result = 0;
    int i, objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK)
	return TCL_ERROR;

    for (i = 0; i < objc; ++i) {
	int index;
	if (TCL_OK != Tcl_GetIndexFromObjStruct(interp, objv[i], table,
		sizeof(char *), "value", TCL_EXACT, &index))
	{
	    return TCL_ERROR;
	}
	result |= (1 << index);
    }

    *resultPtr = result;
    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Treeview widget record.
 *
 * Dependencies:
 * 	columns, columnNames: -columns
 * 	displayColumns:	-columns, -displaycolumns
 * 	headingHeight: [layout]
 * 	rowHeight, indent: style
 */
typedef struct {
    /* Resources acquired at initialization-time:
     */
    Tk_OptionTable itemOptionTable;
    Tk_OptionTable columnOptionTable;
    Tk_OptionTable headingOptionTable;
    Tk_OptionTable tagOptionTable;
    Tk_BindingTable bindingTable;
    Ttk_TagTable tagTable;

    /* Acquired in GetLayout hook:
     */
    Ttk_Layout itemLayout;
    Ttk_Layout cellLayout;
    Ttk_Layout headingLayout;
    Ttk_Layout rowLayout;

    int headingHeight;		/* Space for headings */
    int rowHeight;		/* Height of each item */
    int indent;			/* #pixels horizontal offset for child items */

    /* Tree data:
     */
    Tcl_HashTable items;	/* Map: item name -> item */
    int serial;			/* Next item # for autogenerated names */
    TreeItem *root;		/* Root item */

    TreeColumn column0;		/* Column options for display column #0 */
    TreeColumn *columns;	/* Array of column options for data columns */

    TreeItem *focus;		/* Current focus item */
    TreeItem *endPtr;		/* See EndPosition() */

    /* Widget options:
     */
    Tcl_Obj *columnsObj;	/* List of symbolic column names */
    Tcl_Obj *displayColumnsObj;	/* List of columns to display */

    Tcl_Obj *heightObj;		/* height (rows) */
    Tcl_Obj *paddingObj;	/* internal padding */

    Tcl_Obj *showObj;		/* -show list */
    Tcl_Obj *selectModeObj;	/* -selectmode option */

    Scrollable xscroll;
    ScrollHandle xscrollHandle;
    Scrollable yscroll;
    ScrollHandle yscrollHandle;

    /* Derived resources:
     */
    Tcl_HashTable columnNames;	/* Map: column name -> column table entry */
    int nColumns; 		/* #columns */
    unsigned showFlags;		/* bitmask of subparts to display */

    TreeColumn **displayColumns; /* List of columns for display (incl tree) */
    int nDisplayColumns;	/* #display columns */
    Ttk_Box headingArea;	/* Display area for column headings */
    Ttk_Box treeArea;   	/* Display area for tree */
    int slack;			/* Slack space (see Resizing section) */

} TreePart;

typedef struct {
    WidgetCore core;
    TreePart tree;
} Treeview;

#define USER_MASK 		0x0100
#define COLUMNS_CHANGED 	(USER_MASK)
#define DCOLUMNS_CHANGED	(USER_MASK<<1)
#define SCROLLCMD_CHANGED	(USER_MASK<<2)
#define SHOW_CHANGED 		(USER_MASK<<3)

static const char *SelectModeStrings[] = { "none", "browse", "extended", NULL };

static Tk_OptionSpec TreeviewOptionSpecs[] = {
    {TK_OPTION_STRING, "-columns", "columns", "Columns",
	"", Tk_Offset(Treeview,tree.columnsObj), -1,
	0,0,COLUMNS_CHANGED | GEOMETRY_CHANGED /*| READONLY_OPTION*/ },
    {TK_OPTION_STRING, "-displaycolumns","displayColumns","DisplayColumns",
	"#all", Tk_Offset(Treeview,tree.displayColumnsObj), -1,
	0,0,DCOLUMNS_CHANGED | GEOMETRY_CHANGED },
    {TK_OPTION_STRING, "-show", "show", "Show",
	DEFAULT_SHOW, Tk_Offset(Treeview,tree.showObj), -1,
	0,0,SHOW_CHANGED | GEOMETRY_CHANGED },

    {TK_OPTION_STRING_TABLE, "-selectmode", "selectMode", "SelectMode",
	"extended", Tk_Offset(Treeview,tree.selectModeObj), -1,
	0,(ClientData)SelectModeStrings,0 },

    {TK_OPTION_PIXELS, "-height", "height", "Height",
	DEF_TREE_ROWS, Tk_Offset(Treeview,tree.heightObj), -1,
	0,0,GEOMETRY_CHANGED},
    {TK_OPTION_STRING, "-padding", "padding", "Pad",
	NULL, Tk_Offset(Treeview,tree.paddingObj), -1,
	TK_OPTION_NULL_OK,0,GEOMETRY_CHANGED },

    {TK_OPTION_STRING, "-xscrollcommand", "xScrollCommand", "ScrollCommand",
	NULL, -1, Tk_Offset(Treeview, tree.xscroll.scrollCmd),
	TK_OPTION_NULL_OK, 0, SCROLLCMD_CHANGED},
    {TK_OPTION_STRING, "-yscrollcommand", "yScrollCommand", "ScrollCommand",
	NULL, -1, Tk_Offset(Treeview, tree.yscroll.scrollCmd),
	TK_OPTION_NULL_OK, 0, SCROLLCMD_CHANGED},

    WIDGET_TAKEFOCUS_TRUE,
    WIDGET_INHERIT_OPTIONS(ttkCoreOptionSpecs)
};

/*------------------------------------------------------------------------
 * +++ Utilities.
 */
typedef void (*HashEntryIterator)(void *hashValue);

static void foreachHashEntry(Tcl_HashTable *ht, HashEntryIterator func)
{
    Tcl_HashSearch search;
    Tcl_HashEntry *entryPtr = Tcl_FirstHashEntry(ht, &search);
    while (entryPtr != NULL) {
	func(Tcl_GetHashValue(entryPtr));
	entryPtr = Tcl_NextHashEntry(&search);
    }
}

/* + unshareObj(objPtr) --
 * 	Ensure that a Tcl_Obj * has refcount 1 -- either return objPtr
 * 	itself,	or a duplicated copy.
 */
static Tcl_Obj *unshareObj(Tcl_Obj *objPtr)
{
    if (Tcl_IsShared(objPtr)) {
	Tcl_Obj *newObj = Tcl_DuplicateObj(objPtr);
	Tcl_DecrRefCount(objPtr);
	Tcl_IncrRefCount(newObj);
	return newObj;
    }
    return objPtr;
}

/* DisplayLayout --
 * 	Rebind, place, and draw a layout + object combination.
 */
static void DisplayLayout(
    Ttk_Layout layout, void *recordPtr, Ttk_State state, Ttk_Box b, Drawable d)
{
    Ttk_RebindSublayout(layout, recordPtr);
    Ttk_PlaceLayout(layout, state, b);
    Ttk_DrawLayout(layout, state, d);
}

/* + GetColumn --
 * 	Look up column by name or number.
 * 	Returns: pointer to column table entry, NULL if not found.
 * 	Leaves an error message in interp->result on error.
 */
static TreeColumn *GetColumn(
    Tcl_Interp *interp, Treeview *tv, Tcl_Obj *columnIDObj)
{
    Tcl_HashEntry *entryPtr;
    int columnIndex;

    /* Check for named column:
     */
    entryPtr = Tcl_FindHashEntry(
	    &tv->tree.columnNames, Tcl_GetString(columnIDObj));
    if (entryPtr) {
	return Tcl_GetHashValue(entryPtr);
    }

    /* Check for number:
     */
    if (Tcl_GetIntFromObj(NULL, columnIDObj, &columnIndex) == TCL_OK) {
	if (columnIndex < 0 || columnIndex >= tv->tree.nColumns) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "Column index %s out of bounds",
		    Tcl_GetString(columnIDObj)));
	    Tcl_SetErrorCode(interp, "TTK", "TREE", "COLBOUND", NULL);
	    return NULL;
	}

	return tv->tree.columns + columnIndex;
    }
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	"Invalid column index %s", Tcl_GetString(columnIDObj)));
    Tcl_SetErrorCode(interp, "TTK", "TREE", "COLUMN", NULL);
    return NULL;
}

/* + FindColumn --
 * 	Look up column by name, number, or display index.
 */
static TreeColumn *FindColumn(
    Tcl_Interp *interp, Treeview *tv, Tcl_Obj *columnIDObj)
{
    int colno;

    if (sscanf(Tcl_GetString(columnIDObj), "#%d", &colno) == 1)
    {	/* Display column specification, #n */
	if (colno >= 0 && colno < tv->tree.nDisplayColumns) {
	    return tv->tree.displayColumns[colno];
	}
	/* else */
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "Column %s out of range", Tcl_GetString(columnIDObj)));
	Tcl_SetErrorCode(interp, "TTK", "TREE", "COLUMN", NULL);
	return NULL;
    }

    return GetColumn(interp, tv, columnIDObj);
}

/* + FindItem --
 * 	Locates the item with the specified identifier in the tree.
 * 	If there is no such item, leaves an error message in interp.
 */
static TreeItem *FindItem(
    Tcl_Interp *interp, Treeview *tv, Tcl_Obj *itemNameObj)
{
    const char *itemName = Tcl_GetString(itemNameObj);
    Tcl_HashEntry *entryPtr =  Tcl_FindHashEntry(&tv->tree.items, itemName);

    if (!entryPtr) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Item %s not found", itemName));
	Tcl_SetErrorCode(interp, "TTK", "TREE", "ITEM", NULL);
	return 0;
    }
    return Tcl_GetHashValue(entryPtr);
}

/* + GetItemListFromObj --
 * 	Parse a Tcl_Obj * as a list of items.
 * 	Returns a NULL-terminated array of items; result must
 * 	be ckfree()d. On error, returns NULL and leaves an error
 * 	message in interp.
 */

static TreeItem **GetItemListFromObj(
    Tcl_Interp *interp, Treeview *tv, Tcl_Obj *objPtr)
{
    TreeItem **items;
    Tcl_Obj **elements;
    int i, nElements;

    if (Tcl_ListObjGetElements(interp,objPtr,&nElements,&elements) != TCL_OK) {
	return NULL;
    }

    items = ckalloc((nElements + 1)*sizeof(TreeItem*));
    for (i = 0; i < nElements; ++i) {
	items[i] = FindItem(interp, tv, elements[i]);
	if (!items[i]) {
	    ckfree(items);
	    return NULL;
	}
    }
    items[i] = NULL;
    return items;
}

/* + ItemName --
 * 	Returns the item's ID.
 */
static const char *ItemName(Treeview *tv, TreeItem *item)
{
    return Tcl_GetHashKey(&tv->tree.items, item->entryPtr);
}

/* + ItemID --
 * 	Returns a fresh Tcl_Obj * (refcount 0) holding the
 * 	item identifier of the specified item.
 */
static Tcl_Obj *ItemID(Treeview *tv, TreeItem *item)
{
    return Tcl_NewStringObj(ItemName(tv, item), -1);
}

/*------------------------------------------------------------------------
 * +++ Column configuration.
 */

/* + TreeviewFreeColumns --
 * 	Free column data.
 */
static void TreeviewFreeColumns(Treeview *tv)
{
    int i;

    Tcl_DeleteHashTable(&tv->tree.columnNames);
    Tcl_InitHashTable(&tv->tree.columnNames, TCL_STRING_KEYS);

    if (tv->tree.columns) {
	for (i = 0; i < tv->tree.nColumns; ++i)
	    FreeColumn(tv->tree.columns + i);
	ckfree(tv->tree.columns);
	tv->tree.columns = 0;
    }
}

/* + TreeviewInitColumns --
 *	Initialize column data when -columns changes.
 *	Returns: TCL_OK or TCL_ERROR;
 */
static int TreeviewInitColumns(Tcl_Interp *interp, Treeview *tv)
{
    Tcl_Obj **columns;
    int i, ncols;

    if (Tcl_ListObjGetElements(
	    interp, tv->tree.columnsObj, &ncols, &columns) != TCL_OK)
    {
	return TCL_ERROR;
    }

    /*
     * Free old values:
     */
    TreeviewFreeColumns(tv);

    /*
     * Initialize columns array and columnNames hash table:
     */
    tv->tree.nColumns = ncols;
    tv->tree.columns = ckalloc(tv->tree.nColumns * sizeof(TreeColumn));

    for (i = 0; i < ncols; ++i) {
	int isNew;
	Tcl_Obj *columnName = Tcl_DuplicateObj(columns[i]);

	Tcl_HashEntry *entryPtr = Tcl_CreateHashEntry(
	    &tv->tree.columnNames, Tcl_GetString(columnName), &isNew);
	Tcl_SetHashValue(entryPtr, tv->tree.columns + i);

	InitColumn(tv->tree.columns + i);
	Tk_InitOptions(
	    interp, (ClientData)(tv->tree.columns + i),
	    tv->tree.columnOptionTable, tv->core.tkwin);
	Tk_InitOptions(
	    interp, (ClientData)(tv->tree.columns + i),
	    tv->tree.headingOptionTable, tv->core.tkwin);
	Tcl_IncrRefCount(columnName);
	tv->tree.columns[i].idObj = columnName;
    }

    return TCL_OK;
}

/* + TreeviewInitDisplayColumns --
 * 	Initializes the 'displayColumns' array.
 *
 * 	Note that displayColumns[0] is always the tree column,
 * 	even when SHOW_TREE is not set.
 *
 * @@@ TODO: disallow duplicated columns
 */
static int TreeviewInitDisplayColumns(Tcl_Interp *interp, Treeview *tv)
{
    Tcl_Obj **dcolumns;
    int index, ndcols;
    TreeColumn **displayColumns = 0;

    if (Tcl_ListObjGetElements(interp,
	    tv->tree.displayColumnsObj, &ndcols, &dcolumns) != TCL_OK) {
	return TCL_ERROR;
    }

    if (!strcmp(Tcl_GetString(tv->tree.displayColumnsObj), "#all")) {
	ndcols = tv->tree.nColumns;
	displayColumns = ckalloc((ndcols+1) * sizeof(TreeColumn*));
	for (index = 0; index < ndcols; ++index) {
	    displayColumns[index+1] = tv->tree.columns + index;
	}
    } else {
	displayColumns = ckalloc((ndcols+1) * sizeof(TreeColumn*));
	for (index = 0; index < ndcols; ++index) {
	    displayColumns[index+1] = GetColumn(interp, tv, dcolumns[index]);
	    if (!displayColumns[index+1]) {
		ckfree(displayColumns);
		return TCL_ERROR;
	    }
	}
    }
    displayColumns[0] = &tv->tree.column0;

    if (tv->tree.displayColumns)
	ckfree(tv->tree.displayColumns);
    tv->tree.displayColumns = displayColumns;
    tv->tree.nDisplayColumns = ndcols + 1;

    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Resizing.
 * 	slack invariant: TreeWidth(tree) + slack = treeArea.width
 */

#define FirstColumn(tv)  ((tv->tree.showFlags&SHOW_TREE) ? 0 : 1)

/* + TreeWidth --
 * 	Compute the requested tree width from the sum of visible column widths.
 */
static int TreeWidth(Treeview *tv)
{
    int i = FirstColumn(tv);
    int width = 0;

    while (i < tv->tree.nDisplayColumns) {
	width += tv->tree.displayColumns[i++]->width;
    }
    return width;
}

/* + RecomputeSlack --
 */
static void RecomputeSlack(Treeview *tv)
{
    tv->tree.slack = tv->tree.treeArea.width - TreeWidth(tv);
}

/* + PickupSlack/DepositSlack --
 * 	When resizing columns, distribute extra space to 'slack' first,
 * 	and only adjust column widths if 'slack' goes to zero.
 * 	That is, don't bother changing column widths if the tree
 * 	is already scrolled or short.
 */
static int PickupSlack(Treeview *tv, int extra)
{
    int newSlack = tv->tree.slack + extra;

    if (   (newSlack < 0 && 0 <= tv->tree.slack)
	|| (newSlack > 0 && 0 >= tv->tree.slack))
    {
	tv->tree.slack = 0;
	return newSlack;
    } else {
	tv->tree.slack = newSlack;
	return 0;
    }
}

static void DepositSlack(Treeview *tv, int extra)
{
    tv->tree.slack += extra;
}

/* + Stretch --
 * 	Adjust width of column by N pixels, down to minimum width.
 * 	Returns: #pixels actually moved.
 */
static int Stretch(TreeColumn *c, int n)
{
    int newWidth = n + c->width;
    if (newWidth < c->minWidth) {
	n = c->minWidth - c->width;
	c->width = c->minWidth;
    } else {
	c->width = newWidth;
    }
    return n;
}

/* + ShoveLeft --
 * 	Adjust width of (stretchable) columns to the left by N pixels.
 * 	Returns: leftover slack.
 */
static int ShoveLeft(Treeview *tv, int i, int n)
{
    int first = FirstColumn(tv);
    while (n != 0 && i >= first) {
	TreeColumn *c = tv->tree.displayColumns[i];
	if (c->stretch) {
	    n -= Stretch(c, n);
	}
	--i;
    }
    return n;
}

/* + ShoveRight --
 * 	Adjust width of (stretchable) columns to the right by N pixels.
 * 	Returns: leftover slack.
 */
static int ShoveRight(Treeview *tv, int i, int n)
{
    while (n != 0 && i < tv->tree.nDisplayColumns) {
	TreeColumn *c = tv->tree.displayColumns[i];
	if (c->stretch) {
	    n -= Stretch(c, n);
	}
	++i;
    }
    return n;
}

/* + DistributeWidth --
 * 	Distribute n pixels evenly across all stretchable display columns.
 * 	Returns: leftover slack.
 * Notes:
 * 	The "((++w % m) < r)" term is there so that the remainder r = n % m
 * 	is distributed round-robin.
 */
static int DistributeWidth(Treeview *tv, int n)
{
    int w = TreeWidth(tv);
    int m = 0;
    int i, d, r;

    for (i = FirstColumn(tv); i < tv->tree.nDisplayColumns; ++i) {
	if (tv->tree.displayColumns[i]->stretch) {
	    ++m;
	}
    }
    if (m == 0) {
	return n;
    }

    d = n / m;
    r = n % m;
    if (r < 0) { r += m; --d; }

    for (i = FirstColumn(tv); i < tv->tree.nDisplayColumns; ++i) {
	TreeColumn *c = tv->tree.displayColumns[i];
	if (c->stretch) {
	    n -= Stretch(c, d + ((++w % m) < r));
	}
    }
    return n;
}

/* + ResizeColumns --
 * 	Recompute column widths based on available width.
 * 	Pick up slack first;
 * 	Distribute the remainder evenly across stretchable columns;
 * 	If any is still left over due to minwidth constraints, shove left.
 */
static void ResizeColumns(Treeview *tv, int newWidth)
{
    int delta = newWidth - (TreeWidth(tv) + tv->tree.slack);
    DepositSlack(tv,
	ShoveLeft(tv, tv->tree.nDisplayColumns - 1,
	    DistributeWidth(tv, PickupSlack(tv, delta))));
}

/* + DragColumn --
 * 	Move the separator to the right of specified column,
 * 	adjusting other column widths as necessary.
 */
static void DragColumn(Treeview *tv, int i, int delta)
{
    TreeColumn *c = tv->tree.displayColumns[i];
    int dl = delta - ShoveLeft(tv, i-1, delta - Stretch(c, delta));
    int dr = ShoveRight(tv, i+1, PickupSlack(tv, -dl));
    DepositSlack(tv, dr);
}

/*------------------------------------------------------------------------
 * +++ Event handlers.
 */

static TreeItem *IdentifyItem(Treeview *tv, int y); /*forward*/

static const unsigned long TreeviewBindEventMask =
      KeyPressMask|KeyReleaseMask
    | ButtonPressMask|ButtonReleaseMask
    | PointerMotionMask|ButtonMotionMask
    | VirtualEventMask
    ;

static void TreeviewBindEventProc(void *clientData, XEvent *event)
{
    Treeview *tv = clientData;
    TreeItem *item = NULL;
    Ttk_TagSet tagset;

    /*
     * Figure out where to deliver the event.
     */
    switch (event->type)
    {
	case KeyPress:
	case KeyRelease:
	case VirtualEvent:
	    item = tv->tree.focus;
	    break;
	case ButtonPress:
	case ButtonRelease:
	    item = IdentifyItem(tv, event->xbutton.y);
	    break;
	case MotionNotify:
	    item = IdentifyItem(tv, event->xmotion.y);
	    break;
	default:
	    break;
    }

    if (!item) {
	return;
    }

    /* ASSERT: Ttk_GetTagSetFromObj succeeds.
     * NB: must use a local copy of the tagset,
     * in case a binding script stomps on -tags.
     */
    tagset = Ttk_GetTagSetFromObj(NULL, tv->tree.tagTable, item->tagsObj);

    /*
     * Fire binding:
     */
    Tcl_Preserve(clientData);
    Tk_BindEvent(tv->tree.bindingTable, event, tv->core.tkwin,
	    tagset->nTags, (void **)tagset->tags);
    Tcl_Release(clientData);

    Ttk_FreeTagSet(tagset);
}

/*------------------------------------------------------------------------
 * +++ Initialization and cleanup.
 */

static void TreeviewInitialize(Tcl_Interp *interp, void *recordPtr)
{
    Treeview *tv = recordPtr;
    int unused;

    tv->tree.itemOptionTable =
	Tk_CreateOptionTable(interp, ItemOptionSpecs);
    tv->tree.columnOptionTable =
	Tk_CreateOptionTable(interp, ColumnOptionSpecs);
    tv->tree.headingOptionTable =
	Tk_CreateOptionTable(interp, HeadingOptionSpecs);
    tv->tree.tagOptionTable =
	Tk_CreateOptionTable(interp, TagOptionSpecs);

    tv->tree.tagTable = Ttk_CreateTagTable(
	interp, tv->core.tkwin, TagOptionSpecs, sizeof(DisplayItem));
    tv->tree.bindingTable = Tk_CreateBindingTable(interp);
    Tk_CreateEventHandler(tv->core.tkwin,
	TreeviewBindEventMask, TreeviewBindEventProc, tv);

    tv->tree.itemLayout
	= tv->tree.cellLayout
	= tv->tree.headingLayout
	= tv->tree.rowLayout
	= 0;
    tv->tree.headingHeight = tv->tree.rowHeight = DEFAULT_ROWHEIGHT;
    tv->tree.indent = DEFAULT_INDENT;

    Tcl_InitHashTable(&tv->tree.columnNames, TCL_STRING_KEYS);
    tv->tree.nColumns = tv->tree.nDisplayColumns = 0;
    tv->tree.columns = NULL;
    tv->tree.displayColumns = NULL;
    tv->tree.showFlags = ~0;

    InitColumn(&tv->tree.column0);
    Tk_InitOptions(
	interp, (ClientData)(&tv->tree.column0),
	tv->tree.columnOptionTable, tv->core.tkwin);
    Tk_InitOptions(
	interp, (ClientData)(&tv->tree.column0),
	tv->tree.headingOptionTable, tv->core.tkwin);

    Tcl_InitHashTable(&tv->tree.items, TCL_STRING_KEYS);
    tv->tree.serial = 0;

    tv->tree.focus = tv->tree.endPtr = 0;

    /* Create root item "":
     */
    tv->tree.root = NewItem();
    Tk_InitOptions(interp, (ClientData)tv->tree.root,
	tv->tree.itemOptionTable, tv->core.tkwin);
    tv->tree.root->tagset = Ttk_GetTagSetFromObj(NULL, tv->tree.tagTable, NULL);
    tv->tree.root->entryPtr = Tcl_CreateHashEntry(&tv->tree.items, "", &unused);
    Tcl_SetHashValue(tv->tree.root->entryPtr, tv->tree.root);

    /* Scroll handles:
     */
    tv->tree.xscrollHandle = TtkCreateScrollHandle(&tv->core,&tv->tree.xscroll);
    tv->tree.yscrollHandle = TtkCreateScrollHandle(&tv->core,&tv->tree.yscroll);

    /* Size parameters:
     */
    tv->tree.treeArea = tv->tree.headingArea = Ttk_MakeBox(0,0,0,0);
    tv->tree.slack = 0;
}

static void TreeviewCleanup(void *recordPtr)
{
    Treeview *tv = recordPtr;

    Tk_DeleteEventHandler(tv->core.tkwin,
	    TreeviewBindEventMask,  TreeviewBindEventProc, tv);
    Tk_DeleteBindingTable(tv->tree.bindingTable);
    Ttk_DeleteTagTable(tv->tree.tagTable);

    if (tv->tree.itemLayout) Ttk_FreeLayout(tv->tree.itemLayout);
    if (tv->tree.cellLayout) Ttk_FreeLayout(tv->tree.cellLayout);
    if (tv->tree.headingLayout) Ttk_FreeLayout(tv->tree.headingLayout);
    if (tv->tree.rowLayout) Ttk_FreeLayout(tv->tree.rowLayout);

    TreeviewFreeColumns(tv);

    if (tv->tree.displayColumns)
	ckfree((ClientData)tv->tree.displayColumns);

    foreachHashEntry(&tv->tree.items, FreeItemCB);
    Tcl_DeleteHashTable(&tv->tree.items);

    TtkFreeScrollHandle(tv->tree.xscrollHandle);
    TtkFreeScrollHandle(tv->tree.yscrollHandle);
}

/* + TreeviewConfigure --
 * 	Configuration widget hook.
 *
 * 	BUG: If user sets -columns and -displaycolumns, but -displaycolumns
 * 	has an error, the widget is left in an inconsistent state.
 */
static int
TreeviewConfigure(Tcl_Interp *interp, void *recordPtr, int mask)
{
    Treeview *tv = recordPtr;
    unsigned showFlags = tv->tree.showFlags;

    if (mask & COLUMNS_CHANGED) {
	if (TreeviewInitColumns(interp, tv) != TCL_OK)
	    return TCL_ERROR;
	mask |= DCOLUMNS_CHANGED;
    }
    if (mask & DCOLUMNS_CHANGED) {
	if (TreeviewInitDisplayColumns(interp, tv) != TCL_OK)
	    return TCL_ERROR;
    }
    if (mask & SCROLLCMD_CHANGED) {
	TtkScrollbarUpdateRequired(tv->tree.xscrollHandle);
	TtkScrollbarUpdateRequired(tv->tree.yscrollHandle);
    }
    if (  (mask & SHOW_CHANGED)
	&& GetEnumSetFromObj(
		    interp,tv->tree.showObj,showStrings,&showFlags) != TCL_OK)
    {
	return TCL_ERROR;
    }

    if (TtkCoreConfigure(interp, recordPtr, mask) != TCL_OK) {
	return TCL_ERROR;
    }

    tv->tree.showFlags = showFlags;

    if (mask & (SHOW_CHANGED | DCOLUMNS_CHANGED)) {
	RecomputeSlack(tv);
    }
    return TCL_OK;
}

/* + ConfigureItem --
 * 	Set item options.
 */
static int ConfigureItem(
    Tcl_Interp *interp, Treeview *tv, TreeItem *item,
    int objc, Tcl_Obj *const objv[])
{
    Tk_SavedOptions savedOptions;
    int mask;
    Ttk_ImageSpec *newImageSpec = NULL;
    Ttk_TagSet newTagSet = NULL;

    if (Tk_SetOptions(interp, (ClientData)item, tv->tree.itemOptionTable,
		objc, objv, tv->core.tkwin, &savedOptions, &mask)
		!= TCL_OK)
    {
	return TCL_ERROR;
    }

    /* Make sure that -values is a valid list:
     */
    if (item->valuesObj) {
	int unused;
	if (Tcl_ListObjLength(interp, item->valuesObj, &unused) != TCL_OK)
	    goto error;
    }

    /* Check -image.
     */
    if ((mask & ITEM_OPTION_IMAGE_CHANGED) && item->imageObj) {
	newImageSpec = TtkGetImageSpec(interp, tv->core.tkwin, item->imageObj);
	if (!newImageSpec) {
	    goto error;
	}
    }

    /* Check -tags.
     * Side effect: may create new tags.
     */
    if (mask & ITEM_OPTION_TAGS_CHANGED) {
	newTagSet = Ttk_GetTagSetFromObj(
		interp, tv->tree.tagTable, item->tagsObj);
	if (!newTagSet) {
	    goto error;
	}
    }

    /* Keep TTK_STATE_OPEN flag in sync with item->openObj.
     * We use both a state flag and a Tcl_Obj* resource so elements
     * can access the value in either way.
     */
    if (item->openObj) {
	int isOpen;
	if (Tcl_GetBooleanFromObj(interp, item->openObj, &isOpen) != TCL_OK)
	    goto error;
	if (isOpen)
	    item->state |= TTK_STATE_OPEN;
	else
	    item->state &= ~TTK_STATE_OPEN;
    }

    /* All OK.
     */
    Tk_FreeSavedOptions(&savedOptions);
    if (mask & ITEM_OPTION_TAGS_CHANGED) {
	if (item->tagset) { Ttk_FreeTagSet(item->tagset); }
	item->tagset = newTagSet;
    }
    if (mask & ITEM_OPTION_IMAGE_CHANGED) {
	if (item->imagespec) { TtkFreeImageSpec(item->imagespec); }
	item->imagespec = newImageSpec;
    }
    TtkRedisplayWidget(&tv->core);
    return TCL_OK;

error:
    Tk_RestoreSavedOptions(&savedOptions);
    if (newTagSet) { Ttk_FreeTagSet(newTagSet); }
    if (newImageSpec) { TtkFreeImageSpec(newImageSpec); }
    return TCL_ERROR;
}

/* + ConfigureColumn --
 * 	Set column options.
 */
static int ConfigureColumn(
    Tcl_Interp *interp, Treeview *tv, TreeColumn *column,
    int objc, Tcl_Obj *const objv[])
{
    Tk_SavedOptions savedOptions;
    int mask;

    if (Tk_SetOptions(interp, (ClientData)column,
	    tv->tree.columnOptionTable, objc, objv, tv->core.tkwin,
	    &savedOptions,&mask) != TCL_OK)
    {
	return TCL_ERROR;
    }

    if (mask & READONLY_OPTION) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"Attempt to change read-only option", -1));
	Tcl_SetErrorCode(interp, "TTK", "TREE", "READONLY", NULL);
	goto error;
    }

    /* Propagate column width changes to overall widget request width,
     * but only if the widget is currently unmapped, in order to prevent
     * geometry jumping during interactive column resize.
     */
    if (mask & GEOMETRY_CHANGED) {
	if (!Tk_IsMapped(tv->core.tkwin)) {
	    TtkResizeWidget(&tv->core);
        } else {
	    RecomputeSlack(tv);
	    ResizeColumns(tv, TreeWidth(tv));
        }
    }
    TtkRedisplayWidget(&tv->core);

    Tk_FreeSavedOptions(&savedOptions);
    return TCL_OK;

error:
    Tk_RestoreSavedOptions(&savedOptions);
    return TCL_ERROR;
}

/* + ConfigureHeading --
 * 	Set heading options.
 */
static int ConfigureHeading(
    Tcl_Interp *interp, Treeview *tv, TreeColumn *column,
    int objc, Tcl_Obj *const objv[])
{
    Tk_SavedOptions savedOptions;
    int mask;

    if (Tk_SetOptions(interp, (ClientData)column,
	    tv->tree.headingOptionTable, objc, objv, tv->core.tkwin,
	    &savedOptions,&mask) != TCL_OK)
    {
	return TCL_ERROR;
    }

    /* @@@ testing ... */
    if ((mask & STATE_CHANGED) && column->headingStateObj) {
	Ttk_StateSpec stateSpec;
	if (Ttk_GetStateSpecFromObj(
		interp, column->headingStateObj, &stateSpec) != TCL_OK)
	{
	    goto error;
	}
	column->headingState = Ttk_ModifyState(column->headingState,&stateSpec);
	Tcl_DecrRefCount(column->headingStateObj);
	column->headingStateObj = Ttk_NewStateSpecObj(column->headingState,0);
	Tcl_IncrRefCount(column->headingStateObj);
    }

    TtkRedisplayWidget(&tv->core);
    Tk_FreeSavedOptions(&savedOptions);
    return TCL_OK;

error:
    Tk_RestoreSavedOptions(&savedOptions);
    return TCL_ERROR;
}

/*------------------------------------------------------------------------
 * +++ Geometry routines.
 */

/* + CountRows --
 * 	Returns the number of viewable rows rooted at item
 */
static int CountRows(TreeItem *item)
{
    int rows = 1;

    if (item->state & TTK_STATE_OPEN) {
	TreeItem *child = item->children;
	while (child) {
	    rows += CountRows(child);
	    child = child->next;
	}
    }
    return rows;
}

/* + IdentifyRow --
 * 	Recursive search for item at specified y position.
 * 	Main work routine for IdentifyItem()
 */
static TreeItem *IdentifyRow(
    Treeview *tv,	/* Widget record */
    TreeItem *item, 	/* Where to start search */
    int *ypos,		/* Scan position */
    int y)		/* Target y coordinate */
{
    while (item) {
	int next_ypos = *ypos + tv->tree.rowHeight;
	if (*ypos <= y && y <= next_ypos) {
	    return item;
	}
	*ypos = next_ypos;
	if (item->state & TTK_STATE_OPEN) {
	    TreeItem *subitem = IdentifyRow(tv, item->children, ypos, y);
	    if (subitem) {
		return subitem;
	    }
	}
	item = item->next;
    }
    return 0;
}

/* + IdentifyItem --
 * 	Locate the item at the specified y position, if any.
 */
static TreeItem *IdentifyItem(Treeview *tv, int y)
{
    int rowHeight = tv->tree.rowHeight;
    int ypos = tv->tree.treeArea.y - rowHeight * tv->tree.yscroll.first;
    return IdentifyRow(tv, tv->tree.root->children, &ypos, y);
}

/* + IdentifyDisplayColumn --
 * 	Returns the display column number at the specified x position,
 * 	or -1 if x is outside any columns.
 */
static int IdentifyDisplayColumn(Treeview *tv, int x, int *x1)
{
    int colno = FirstColumn(tv);
    int xpos = tv->tree.treeArea.x - tv->tree.xscroll.first;

    while (colno < tv->tree.nDisplayColumns) {
	TreeColumn *column = tv->tree.displayColumns[colno];
	int next_xpos = xpos + column->width;
	if (xpos <= x && x <= next_xpos + HALO) {
	    *x1 = next_xpos;
	    return colno;
	}
	++colno;
	xpos = next_xpos;
    }

    return -1;
}

/* + RowNumber --
 * 	Calculate which row the specified item appears on;
 * 	returns -1 if the item is not viewable.
 * 	Xref: DrawForest, IdentifyItem.
 */
static int RowNumber(Treeview *tv, TreeItem *item)
{
    TreeItem *p = tv->tree.root->children;
    int n = 0;

    while (p) {
	if (p == item)
	    return n;

	++n;

	/* Find next viewable item in preorder traversal order
	 */
	if (p->children && (p->state & TTK_STATE_OPEN)) {
	    p = p->children;
	} else {
	    while (!p->next && p && p->parent)
		p = p->parent;
	    if (p)
		p = p->next;
	}
    }

    return -1;
}

/* + ItemDepth -- return the depth of a tree item.
 * 	The depth of an item is equal to the number of proper ancestors,
 * 	not counting the root node.
 */
static int ItemDepth(TreeItem *item)
{
    int depth = 0;
    while (item->parent) {
	++depth;
	item = item->parent;
    }
    return depth-1;
}

/* + ItemRow --
 * 	Returns row number of specified item relative to root,
 * 	-1 if item is not viewable.
 */
static int ItemRow(Treeview *tv, TreeItem *p)
{
    TreeItem *root = tv->tree.root;
    int rowNumber = 0;

    for (;;) {
	if (p->prev) {
	    p = p->prev;
	    rowNumber += CountRows(p);
	} else {
	    p = p->parent;
	    if (!(p && (p->state & TTK_STATE_OPEN))) {
		/* detached or closed ancestor */
		return -1;
	    }
	    if (p == root) {
		return rowNumber;
	    }
	    ++rowNumber;
	}
    }
}

/* + BoundingBox --
 * 	Compute the parcel of the specified column of the specified item,
 *	(or the entire item if column is NULL)
 *	Returns: 0 if item or column is not viewable, 1 otherwise.
 */
static int BoundingBox(
    Treeview *tv,		/* treeview widget */
    TreeItem *item,		/* desired item */
    TreeColumn *column,		/* desired column */
    Ttk_Box *bbox_rtn)		/* bounding box of item */
{
    int row = ItemRow(tv, item);
    Ttk_Box bbox = tv->tree.treeArea;

    if (row < tv->tree.yscroll.first || row > tv->tree.yscroll.last) {
	/* not viewable, or off-screen */
	return 0;
    }

    bbox.y += (row - tv->tree.yscroll.first) * tv->tree.rowHeight;
    bbox.height = tv->tree.rowHeight;

    bbox.x -= tv->tree.xscroll.first;
    bbox.width = TreeWidth(tv);

    if (column) {
	int xpos = 0, i = FirstColumn(tv);
	while (i < tv->tree.nDisplayColumns) {
	    if (tv->tree.displayColumns[i] == column) {
		break;
	    }
	    xpos += tv->tree.displayColumns[i]->width;
	    ++i;
	}
	if (i == tv->tree.nDisplayColumns) { /* specified column unviewable */
	    return 0;
	}
	bbox.x += xpos;
	bbox.width = column->width;

	/* Account for indentation in tree column:
	 */
	if (column == &tv->tree.column0) {
	    int indent = tv->tree.indent * ItemDepth(item);
	    bbox.x += indent;
	    bbox.width -= indent;
	}
    }
    *bbox_rtn = bbox;
    return 1;
}

/* + IdentifyRegion --
 */

typedef enum {
    REGION_NOTHING = 0,
    REGION_HEADING,
    REGION_SEPARATOR,
    REGION_TREE,
    REGION_CELL
} TreeRegion;

static const char *regionStrings[] = {
    "nothing", "heading", "separator", "tree", "cell", 0
};

static TreeRegion IdentifyRegion(Treeview *tv, int x, int y)
{
    int x1 = 0, colno;

    colno = IdentifyDisplayColumn(tv, x, &x1);
    if (Ttk_BoxContains(tv->tree.headingArea, x, y)) {
	if (colno < 0) {
	    return REGION_NOTHING;
	} else if (-HALO <= x1 - x  && x1 - x <= HALO) {
	    return REGION_SEPARATOR;
	} else {
	    return REGION_HEADING;
	}
    } else if (Ttk_BoxContains(tv->tree.treeArea, x, y)) {
	TreeItem *item = IdentifyItem(tv, y);
	if (item && colno > 0) {
	    return REGION_CELL;
	} else if (item) {
	    return REGION_TREE;
	}
    }
    return REGION_NOTHING;
}

/*------------------------------------------------------------------------
 * +++ Display routines.
 */

/* + GetSublayout --
 * 	Utility routine; acquires a sublayout for items, cells, etc.
 */
static Ttk_Layout GetSublayout(
    Tcl_Interp *interp,
    Ttk_Theme themePtr,
    Ttk_Layout parentLayout,
    const char *layoutName,
    Tk_OptionTable optionTable,
    Ttk_Layout *layoutPtr)
{
    Ttk_Layout newLayout = Ttk_CreateSublayout(
	    interp, themePtr, parentLayout, layoutName, optionTable);

    if (newLayout) {
	if (*layoutPtr)
	    Ttk_FreeLayout(*layoutPtr);
	*layoutPtr = newLayout;
    }
    return newLayout;
}

/* + TreeviewGetLayout --
 * 	GetLayout() widget hook.
 */
static Ttk_Layout TreeviewGetLayout(
    Tcl_Interp *interp, Ttk_Theme themePtr, void *recordPtr)
{
    Treeview *tv = recordPtr;
    Ttk_Layout treeLayout = TtkWidgetGetLayout(interp, themePtr, recordPtr);
    Tcl_Obj *objPtr;
    int unused;

    if (!(
	treeLayout
     && GetSublayout(interp, themePtr, treeLayout, ".Item",
	    tv->tree.tagOptionTable, &tv->tree.itemLayout)
     && GetSublayout(interp, themePtr, treeLayout, ".Cell",
	    tv->tree.tagOptionTable, &tv->tree.cellLayout)
     && GetSublayout(interp, themePtr, treeLayout, ".Heading",
	    tv->tree.headingOptionTable, &tv->tree.headingLayout)
     && GetSublayout(interp, themePtr, treeLayout, ".Row",
	    tv->tree.tagOptionTable, &tv->tree.rowLayout)
    )) {
	return 0;
    }

    /* Compute heading height.
     */
    Ttk_RebindSublayout(tv->tree.headingLayout, &tv->tree.column0);
    Ttk_LayoutSize(tv->tree.headingLayout, 0, &unused, &tv->tree.headingHeight);

    /* Get item height, indent from style:
     * @@@ TODO: sanity-check.
     */
    tv->tree.rowHeight = DEFAULT_ROWHEIGHT;
    tv->tree.indent = DEFAULT_INDENT;
    if ((objPtr = Ttk_QueryOption(treeLayout, "-rowheight", 0))) {
	(void)Tcl_GetIntFromObj(NULL, objPtr, &tv->tree.rowHeight);
    }
    if ((objPtr = Ttk_QueryOption(treeLayout, "-indent", 0))) {
	(void)Tcl_GetIntFromObj(NULL, objPtr, &tv->tree.indent);
    }

    return treeLayout;
}

/* + TreeviewDoLayout --
 * 	DoLayout() widget hook.  Computes widget layout.
 *
 * Side effects:
 * 	Computes headingArea and treeArea.
 * 	Computes subtree height.
 * 	Invokes scroll callbacks.
 */
static void TreeviewDoLayout(void *clientData)
{
    Treeview *tv = clientData;
    int visibleRows;

    Ttk_PlaceLayout(tv->core.layout,tv->core.state,Ttk_WinBox(tv->core.tkwin));
    tv->tree.treeArea = Ttk_ClientRegion(tv->core.layout, "treearea");

    ResizeColumns(tv, tv->tree.treeArea.width);

    TtkScrolled(tv->tree.xscrollHandle,
	    tv->tree.xscroll.first,
	    tv->tree.xscroll.first + tv->tree.treeArea.width,
	    TreeWidth(tv));

    if (tv->tree.showFlags & SHOW_HEADINGS) {
	tv->tree.headingArea = Ttk_PackBox(
	    &tv->tree.treeArea, 1, tv->tree.headingHeight, TTK_SIDE_TOP);
    } else {
	tv->tree.headingArea = Ttk_MakeBox(0,0,0,0);
    }

    visibleRows = tv->tree.treeArea.height / tv->tree.rowHeight;
    tv->tree.root->state |= TTK_STATE_OPEN;
    TtkScrolled(tv->tree.yscrollHandle,
	    tv->tree.yscroll.first,
	    tv->tree.yscroll.first + visibleRows,
	    CountRows(tv->tree.root) - 1);
}

/* + TreeviewSize --
 * 	SizeProc() widget hook.  Size is determined by
 * 	-height option and column widths.
 */
static int TreeviewSize(void *clientData, int *widthPtr, int *heightPtr)
{
    Treeview *tv = clientData;
    int nRows, padHeight, padWidth;

    Ttk_LayoutSize(tv->core.layout, tv->core.state, &padWidth, &padHeight);
    Tcl_GetIntFromObj(NULL, tv->tree.heightObj, &nRows);

    *widthPtr = padWidth + TreeWidth(tv);
    *heightPtr = padHeight + tv->tree.rowHeight * nRows;

    if (tv->tree.showFlags & SHOW_HEADINGS) {
	*heightPtr += tv->tree.headingHeight;
    }

    return 1;
}

/* + ItemState --
 * 	Returns the state of the specified item, based
 * 	on widget state, item state, and other information.
 */
static Ttk_State ItemState(Treeview *tv, TreeItem *item)
{
    Ttk_State state = tv->core.state | item->state;
    if (!item->children)
	state |= TTK_STATE_LEAF;
    if (item != tv->tree.focus)
	state &= ~TTK_STATE_FOCUS;
    return state;
}

/* + DrawHeadings --
 *	Draw tree headings.
 */
static void DrawHeadings(Treeview *tv, Drawable d)
{
    const int x0 = tv->tree.headingArea.x - tv->tree.xscroll.first;
    const int y0 = tv->tree.headingArea.y;
    const int h0 = tv->tree.headingArea.height;
    int i = FirstColumn(tv);
    int x = 0;

    while (i < tv->tree.nDisplayColumns) {
	TreeColumn *column = tv->tree.displayColumns[i];
	Ttk_Box parcel = Ttk_MakeBox(x0+x, y0, column->width, h0);
	DisplayLayout(tv->tree.headingLayout,
	    column, column->headingState, parcel, d);
	x += column->width;
	++i;
    }
}

/* + PrepareItem --
 * 	Fill in a displayItem record.
 */
static void PrepareItem(
    Treeview *tv, TreeItem *item, DisplayItem *displayItem)
{
    Ttk_Style style = Ttk_LayoutStyle(tv->core.layout);
    Ttk_State state = ItemState(tv, item);

    Ttk_TagSetValues(tv->tree.tagTable, item->tagset, displayItem);
    Ttk_TagSetApplyStyle(tv->tree.tagTable, style, state, displayItem);
}

/* + DrawCells --
 *	Draw data cells for specified item.
 */
static void DrawCells(
    Treeview *tv, TreeItem *item, DisplayItem *displayItem,
    Drawable d, int x, int y)
{
    Ttk_Layout layout = tv->tree.cellLayout;
    Ttk_State state = ItemState(tv, item);
    Ttk_Padding cellPadding = {4, 0, 4, 0};
    int rowHeight = tv->tree.rowHeight;
    int nValues = 0;
    Tcl_Obj **values = 0;
    int i;

    if (!item->valuesObj) {
	return;
    }

    Tcl_ListObjGetElements(NULL, item->valuesObj, &nValues, &values);
    for (i = 0; i < tv->tree.nColumns; ++i) {
	tv->tree.columns[i].data = (i < nValues) ? values[i] : 0;
    }

    for (i = 1; i < tv->tree.nDisplayColumns; ++i) {
	TreeColumn *column = tv->tree.displayColumns[i];
	Ttk_Box parcel = Ttk_PadBox(
	    Ttk_MakeBox(x, y, column->width, rowHeight), cellPadding);

	displayItem->textObj = column->data;
	displayItem->anchorObj = column->anchorObj;	/* <<NOTE-ANCHOR>> */

	DisplayLayout(layout, displayItem, state, parcel, d);
	x += column->width;
    }
}

/* + DrawItem --
 * 	Draw an item (row background, tree label, and cells).
 */
static void DrawItem(
    Treeview *tv, TreeItem *item, Drawable d, int depth, int row)
{
    Ttk_State state = ItemState(tv, item);
    DisplayItem displayItem;
    int rowHeight = tv->tree.rowHeight;
    int x = tv->tree.treeArea.x - tv->tree.xscroll.first;
    int y = tv->tree.treeArea.y + rowHeight * (row - tv->tree.yscroll.first);

    if (row % 2) state |= TTK_STATE_ALTERNATE;

    PrepareItem(tv, item, &displayItem);

    /* Draw row background:
     */
    {
	Ttk_Box rowBox = Ttk_MakeBox(x, y, TreeWidth(tv), rowHeight);
	DisplayLayout(tv->tree.rowLayout, &displayItem, state, rowBox, d);
    }

    /* Draw tree label:
     */
    if (tv->tree.showFlags & SHOW_TREE) {
	int indent = depth * tv->tree.indent;
	int colwidth = tv->tree.column0.width;
	Ttk_Box parcel = Ttk_MakeBox(
		x+indent, y, colwidth-indent, rowHeight);
	if (item->textObj) { displayItem.textObj = item->textObj; }
	if (item->imageObj) { displayItem.imageObj = item->imageObj; }
	/* ??? displayItem.anchorObj = 0; <<NOTE-ANCHOR>> */
	DisplayLayout(tv->tree.itemLayout, &displayItem, state, parcel, d);
	x += colwidth;
    }

    /* Draw data cells:
     */
    DrawCells(tv, item, &displayItem, d, x, y);
}

/* + DrawSubtree --
 * 	Draw an item and all of its (viewable) descendants.
 *
 * Returns:
 * 	Row number of the last item drawn.
 */

static int DrawForest(	/* forward */
    Treeview *tv, TreeItem *item, Drawable d, int depth, int row);

static int DrawSubtree(
    Treeview *tv, TreeItem *item, Drawable d, int depth, int row)
{
    if (row >= tv->tree.yscroll.first) {
	DrawItem(tv, item, d, depth, row);
    }

    if (item->state & TTK_STATE_OPEN) {
	return DrawForest(tv, item->children, d, depth + 1, row + 1);
    } else {
	return row + 1;
    }
}

/* + DrawForest --
 * 	Draw a sequence of items and their visible descendants.
 *
 * Returns:
 * 	Row number of the last item drawn.
 */
static int DrawForest(
    Treeview *tv, TreeItem *item, Drawable d, int depth, int row)
{
    while (item && row < tv->tree.yscroll.last) {
        row = DrawSubtree(tv, item, d, depth, row);
	item = item->next;
    }
    return row;
}

/* + TreeviewDisplay --
 * 	Display() widget hook.  Draw the widget contents.
 */
static void TreeviewDisplay(void *clientData, Drawable d)
{
    Treeview *tv = clientData;

    Ttk_DrawLayout(tv->core.layout, tv->core.state, d);
    if (tv->tree.showFlags & SHOW_HEADINGS) {
	DrawHeadings(tv, d);
    }
    DrawForest(tv, tv->tree.root->children, d, 0,0);
}

/*------------------------------------------------------------------------
 * +++ Utilities for widget commands
 */

/* + InsertPosition --
 * 	Locate the previous sibling for [$tree insert].
 *
 * 	Returns a pointer to the item just before the specified index,
 * 	or 0 if the item is to be inserted at the beginning.
 */
static TreeItem *InsertPosition(TreeItem *parent, int index)
{
    TreeItem *prev = 0, *next = parent->children;

    while (next != 0 && index > 0) {
	--index;
	prev = next;
	next = prev->next;
    }

    return prev;
}

/* + EndPosition --
 * 	Locate the last child of the specified node.
 *
 * 	To avoid quadratic-time behavior in the common cases
 * 	where the treeview is populated in breadth-first or
 * 	depth-first order using [$tv insert $parent end ...],
 * 	we cache the result from the last call to EndPosition()
 * 	and start the search from there on a cache hit.
 *
 */
static TreeItem *EndPosition(Treeview *tv, TreeItem *parent)
{
    TreeItem *endPtr = tv->tree.endPtr;

    while (endPtr && endPtr->parent != parent) {
	endPtr = endPtr->parent;
    }
    if (!endPtr) {
	endPtr = parent->children;
    }

    if (endPtr) {
	while (endPtr->next) {
	    endPtr = endPtr->next;
	}
	tv->tree.endPtr = endPtr;
    }

    return endPtr;
}

/* + AncestryCheck --
 * 	Verify that specified item is not an ancestor of the specified parent;
 * 	returns 1 if OK, 0 and leaves an error message in interp otherwise.
 */
static int AncestryCheck(
    Tcl_Interp *interp, Treeview *tv, TreeItem *item, TreeItem *parent)
{
    TreeItem *p = parent;
    while (p) {
	if (p == item) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "Cannot insert %s as descendant of %s",
		    ItemName(tv, item), ItemName(tv, parent)));
	    Tcl_SetErrorCode(interp, "TTK", "TREE", "ANCESTRY", NULL);
	    return 0;
	}
	p = p->parent;
    }
    return 1;
}

/* + DeleteItems --
 * 	Remove an item and all of its descendants from the hash table
 * 	and detach them from the tree; returns a linked list (chained
 * 	along the ->next pointer) of deleted items.
 */
static TreeItem *DeleteItems(TreeItem *item, TreeItem *delq)
{
    if (item->entryPtr) {
	DetachItem(item);
	while (item->children) {
	    delq = DeleteItems(item->children, delq);
	}
	Tcl_DeleteHashEntry(item->entryPtr);
	item->entryPtr = 0;
	item->next = delq;
	delq = item;
    } /* else -- item has already been unlinked */
    return delq;
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- item inquiry.
 */

/* + $tv children $item ?newchildren? --
 * 	Return the list of children associated with $item
 */
static int TreeviewChildrenCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;
    Tcl_Obj *result;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "item ?newchildren?");
	return TCL_ERROR;
    }
    item = FindItem(interp, tv, objv[2]);
    if (!item) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	result = Tcl_NewListObj(0,0);
	for (item = item->children; item; item = item->next) {
	    Tcl_ListObjAppendElement(interp, result, ItemID(tv, item));
	}
	Tcl_SetObjResult(interp, result);
    } else {
	TreeItem **newChildren = GetItemListFromObj(interp, tv, objv[3]);
	TreeItem *child;
	int i;

	if (!newChildren)
	    return TCL_ERROR;

	/* Sanity-check:
	 */
	for (i=0; newChildren[i]; ++i) {
	    if (!AncestryCheck(interp, tv, newChildren[i], item)) {
		ckfree(newChildren);
		return TCL_ERROR;
	    }
	}

	/* Detach old children:
	 */
	child = item->children;
	while (child) {
	    TreeItem *next = child->next;
	    DetachItem(child);
	    child = next;
	}

	/* Detach new children from their current locations:
	 */
	for (i=0; newChildren[i]; ++i) {
	    DetachItem(newChildren[i]);
	}

	/* Reinsert new children:
	 * Note: it is not an error for an item to be listed more than once,
	 * though it probably should be...
	 */
	child = 0;
	for (i=0; newChildren[i]; ++i) {
	    if (newChildren[i]->parent) {
		/* This is a duplicate element which has already been
		 * inserted.  Ignore it.
		 */
		continue;
	    }
	    InsertItem(item, child, newChildren[i]);
	    child = newChildren[i];
	}

	ckfree(newChildren);
	TtkRedisplayWidget(&tv->core);
    }

    return TCL_OK;
}

/* + $tv parent $item --
 * 	Return the item ID of $item's parent.
 */
static int TreeviewParentCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item");
	return TCL_ERROR;
    }
    item = FindItem(interp, tv, objv[2]);
    if (!item) {
	return TCL_ERROR;
    }

    if (item->parent) {
	Tcl_SetObjResult(interp, ItemID(tv, item->parent));
    } else {
	/* This is the root item.  @@@ Return an error? */
	Tcl_ResetResult(interp);
    }

    return TCL_OK;
}

/* + $tv next $item
 * 	Return the ID of $item's next sibling.
 */
static int TreeviewNextCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item");
	return TCL_ERROR;
    }
    item = FindItem(interp, tv, objv[2]);
    if (!item) {
	return TCL_ERROR;
    }

    if (item->next) {
	Tcl_SetObjResult(interp, ItemID(tv, item->next));
    } /* else -- leave interp-result empty */

    return TCL_OK;
}

/* + $tv prev $item
 * 	Return the ID of $item's previous sibling.
 */
static int TreeviewPrevCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item");
	return TCL_ERROR;
    }
    item = FindItem(interp, tv, objv[2]);
    if (!item) {
	return TCL_ERROR;
    }

    if (item->prev) {
	Tcl_SetObjResult(interp, ItemID(tv, item->prev));
    } /* else -- leave interp-result empty */

    return TCL_OK;
}

/* + $tv index $item --
 * 	Return the index of $item within its parent.
 */
static int TreeviewIndexCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;
    int index = 0;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item");
	return TCL_ERROR;
    }
    item = FindItem(interp, tv, objv[2]);
    if (!item) {
	return TCL_ERROR;
    }

    while (item->prev) {
	++index;
	item = item->prev;
    }

    Tcl_SetObjResult(interp, Tcl_NewIntObj(index));
    return TCL_OK;
}

/* + $tv exists $itemid --
 * 	Test if the specified item id is present in the tree.
 */
static int TreeviewExistsCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    Tcl_HashEntry *entryPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "itemid");
	return TCL_ERROR;
    }

    entryPtr = Tcl_FindHashEntry(&tv->tree.items, Tcl_GetString(objv[2]));
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(entryPtr != 0));
    return TCL_OK;
}

/* + $tv bbox $itemid ?$column? --
 * 	Return bounding box [x y width height] of specified item.
 */
static int TreeviewBBoxCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item = 0;
    TreeColumn *column = 0;
    Ttk_Box bbox;

    if (objc < 3 || objc > 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "itemid ?column");
	return TCL_ERROR;
    }

    item = FindItem(interp, tv, objv[2]);
    if (!item) {
	return TCL_ERROR;
    }
    if (objc >=4 && (column = FindColumn(interp,tv,objv[3])) == NULL) {
	return TCL_ERROR;
    }

    if (BoundingBox(tv, item, column, &bbox)) {
	Tcl_SetObjResult(interp, Ttk_NewBoxObj(bbox));
    }

    return TCL_OK;
}

/* + $tv identify $x $y -- (obsolescent)
 * 	Implements the old, horrible, 2-argument form of [$tv identify].
 *
 * Returns: one of
 * 	heading #n
 * 	cell itemid #n
 * 	item itemid element
 * 	row itemid
 */
static int TreeviewHorribleIdentify(
    Tcl_Interp *interp, int objc, Tcl_Obj *const objv[], Treeview *tv)
{
    const char *what = "nothing", *detail = NULL;
    TreeItem *item = 0;
    Tcl_Obj *result;
    int dColumnNumber;
    char dcolbuf[16];
    int x, y, x1;

    /* ASSERT: objc == 4 */

    if (   Tcl_GetIntFromObj(interp, objv[2], &x) != TCL_OK
	|| Tcl_GetIntFromObj(interp, objv[3], &y) != TCL_OK
    ) {
	return TCL_ERROR;
    }

    dColumnNumber = IdentifyDisplayColumn(tv, x, &x1);
    if (dColumnNumber < 0) {
	goto done;
    }
    sprintf(dcolbuf, "#%d", dColumnNumber);

    if (Ttk_BoxContains(tv->tree.headingArea,x,y)) {
	if (-HALO <= x1 - x  && x1 - x <= HALO) {
	    what = "separator";
	} else {
	    what = "heading";
	}
	detail = dcolbuf;
    } else if (Ttk_BoxContains(tv->tree.treeArea,x,y)) {
	item = IdentifyItem(tv, y);
	if (item && dColumnNumber > 0) {
	    what = "cell";
	    detail = dcolbuf;
	} else if (item) {
	    Ttk_Layout layout = tv->tree.itemLayout;
	    Ttk_Box itemBox;
	    DisplayItem displayItem;
	    Ttk_Element element;

	    BoundingBox(tv, item, NULL, &itemBox);
	    PrepareItem(tv, item, &displayItem);
            if (item->textObj) { displayItem.textObj = item->textObj; }
            if (item->imageObj) { displayItem.imageObj = item->imageObj; }
	    Ttk_RebindSublayout(layout, &displayItem);
	    Ttk_PlaceLayout(layout, ItemState(tv,item), itemBox);
	    element = Ttk_IdentifyElement(layout, x, y);

	    if (element) {
		what = "item";
		detail = Ttk_ElementName(element);
	    } else {
		what = "row";
	    }
	}
    }

done:
    result = Tcl_NewListObj(0,0);
    Tcl_ListObjAppendElement(NULL, result, Tcl_NewStringObj(what, -1));
    if (item)
	Tcl_ListObjAppendElement(NULL, result, ItemID(tv, item));
    if (detail)
	Tcl_ListObjAppendElement(NULL, result, Tcl_NewStringObj(detail, -1));

    Tcl_SetObjResult(interp, result);
    return TCL_OK;
}

/* + $tv identify $component $x $y --
 * 	Identify the component at position x,y.
 */

static int TreeviewIdentifyCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    static const char *submethodStrings[] =
	 { "region", "item", "column", "row", "element", NULL };
    enum { I_REGION, I_ITEM, I_COLUMN, I_ROW, I_ELEMENT };

    Treeview *tv = recordPtr;
    int submethod;
    int x, y;

    TreeRegion region;
    Ttk_Box bbox;
    TreeItem *item;
    TreeColumn *column = 0;
    int colno, x1;

    if (objc == 4) {	/* Old form */
	return TreeviewHorribleIdentify(interp, objc, objv, tv);
    } else if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "command x y");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], submethodStrings,
		sizeof(char *), "command", TCL_EXACT, &submethod) != TCL_OK
        || Tcl_GetIntFromObj(interp, objv[3], &x) != TCL_OK
	|| Tcl_GetIntFromObj(interp, objv[4], &y) != TCL_OK
    ) {
	return TCL_ERROR;
    }

    region = IdentifyRegion(tv, x, y);
    item = IdentifyItem(tv, y);
    colno = IdentifyDisplayColumn(tv, x, &x1);
    column = (colno >= 0) ?  tv->tree.displayColumns[colno] : NULL;

    switch (submethod)
    {
	case I_REGION :
	    Tcl_SetObjResult(interp,Tcl_NewStringObj(regionStrings[region],-1));
	    break;

	case I_ITEM :
	case I_ROW :
	    if (item) {
		Tcl_SetObjResult(interp, ItemID(tv, item));
	    }
	    break;

	case I_COLUMN :
	    if (colno >= 0) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf("#%d", colno));
	    }
	    break;

	case I_ELEMENT :
	{
	    Ttk_Layout layout = 0;
	    DisplayItem displayItem;
	    Ttk_Element element;

	    switch (region) {
		case REGION_NOTHING:
		    layout = tv->core.layout;
		    return TCL_OK; /* @@@ NYI */
		case REGION_HEADING:
		case REGION_SEPARATOR:
		    layout = tv->tree.headingLayout;
		    return TCL_OK; /* @@@ NYI */
		case REGION_TREE:
		    layout = tv->tree.itemLayout;
		    break;
		case REGION_CELL:
		    layout = tv->tree.cellLayout;
		    break;
	    }

	    if (!BoundingBox(tv, item, column, &bbox)) {
		return TCL_OK;
	    }

	    PrepareItem(tv, item, &displayItem);
            if (item->textObj) { displayItem.textObj = item->textObj; }
            if (item->imageObj) { displayItem.imageObj = item->imageObj; }
	    Ttk_RebindSublayout(layout, &displayItem);
	    Ttk_PlaceLayout(layout, ItemState(tv,item), bbox);
	    element = Ttk_IdentifyElement(layout, x, y);

	    if (element) {
		const char *elementName = Ttk_ElementName(element);
		Tcl_SetObjResult(interp, Tcl_NewStringObj(elementName, -1));
	    }
	    break;
	}
    }
    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- item and column configuration.
 */

/* + $tv item $item ?options ....?
 * 	Query or configure item options.
 */
static int TreeviewItemCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item ?option ?value??...");
	return TCL_ERROR;
    }
    if (!(item = FindItem(interp, tv, objv[2]))) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	return TtkEnumerateOptions(interp, item, ItemOptionSpecs,
	    tv->tree.itemOptionTable,  tv->core.tkwin);
    } else if (objc == 4) {
	return TtkGetOptionValue(interp, item, objv[3],
	    tv->tree.itemOptionTable, tv->core.tkwin);
    } else {
	return ConfigureItem(interp, tv, item, objc-3, objv+3);
    }
}

/* + $tv column column ?options ....?
 * 	Column data accessor
 */
static int TreeviewColumnCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeColumn *column;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "column -option value...");
	return TCL_ERROR;
    }
    if (!(column = FindColumn(interp, tv, objv[2]))) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	return TtkEnumerateOptions(interp, column, ColumnOptionSpecs,
	    tv->tree.columnOptionTable, tv->core.tkwin);
    } else if (objc == 4) {
	return TtkGetOptionValue(interp, column, objv[3],
	    tv->tree.columnOptionTable, tv->core.tkwin);
    } else {
	return ConfigureColumn(interp, tv, column, objc-3, objv+3);
    }
}

/* + $tv heading column ?options ....?
 * 	Heading data accessor
 */
static int TreeviewHeadingCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    Tk_OptionTable optionTable = tv->tree.headingOptionTable;
    Tk_Window tkwin = tv->core.tkwin;
    TreeColumn *column;

    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "column -option value...");
	return TCL_ERROR;
    }
    if (!(column = FindColumn(interp, tv, objv[2]))) {
	return TCL_ERROR;
    }

    if (objc == 3) {
	return TtkEnumerateOptions(
	    interp, column, HeadingOptionSpecs, optionTable, tkwin);
    } else if (objc == 4) {
	return TtkGetOptionValue(
	    interp, column, objv[3], optionTable, tkwin);
    } else {
	return ConfigureHeading(interp, tv, column, objc-3,objv+3);
    }
}

/* + $tv set $item ?$column ?value??
 * 	Query or configure cell values
 */
static int TreeviewSetCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item;
    TreeColumn *column;
    int columnNumber;

    if (objc < 3 || objc > 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "item ?column ?value??");
	return TCL_ERROR;
    }
    if (!(item = FindItem(interp, tv, objv[2])))
	return TCL_ERROR;

    /* Make sure -values exists:
     */
    if (!item->valuesObj) {
	item->valuesObj = Tcl_NewListObj(0,0);
	Tcl_IncrRefCount(item->valuesObj);
    }

    if (objc == 3) {
	/* Return dictionary:
	 */
	Tcl_Obj *result = Tcl_NewListObj(0,0);
	Tcl_Obj *value;
	for (columnNumber=0; columnNumber<tv->tree.nColumns; ++columnNumber) {
	    Tcl_ListObjIndex(interp, item->valuesObj, columnNumber, &value);
	    if (value) {
		Tcl_ListObjAppendElement(NULL, result,
			tv->tree.columns[columnNumber].idObj);
		Tcl_ListObjAppendElement(NULL, result, value);
	    }
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    }

    /* else -- get or set column
     */
    if (!(column = FindColumn(interp, tv, objv[3])))
	return TCL_ERROR;

    if (column == &tv->tree.column0) {
	/* @@@ Maybe set -text here instead? */
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"Display column #0 cannot be set", -1));
	Tcl_SetErrorCode(interp, "TTK", "TREE", "COLUMN_0", NULL);
	return TCL_ERROR;
    }

    /* Note: we don't do any error checking in the list operations,
     * since item->valuesObj is guaranteed to be a list.
     */
    columnNumber = column - tv->tree.columns;

    if (objc == 4) {	/* get column */
	Tcl_Obj *result = 0;
	Tcl_ListObjIndex(interp, item->valuesObj, columnNumber, &result);
	if (!result) {
	    result = Tcl_NewStringObj("",0);
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    } else {		/* set column */
	int length;

	item->valuesObj = unshareObj(item->valuesObj);

	/* Make sure -values is fully populated:
	 */
	Tcl_ListObjLength(interp, item->valuesObj, &length);
	while (length < tv->tree.nColumns) {
	    Tcl_Obj *empty = Tcl_NewStringObj("",0);
	    Tcl_ListObjAppendElement(interp, item->valuesObj, empty);
	    ++length;
	}

	/* Set value:
	 */
	Tcl_ListObjReplace(interp,item->valuesObj,columnNumber,1,1,objv+4);
	TtkRedisplayWidget(&tv->core);
	return TCL_OK;
    }
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- tree modification.
 */

/* + $tv insert $parent $index ?-id id? ?-option value ...?
 * 	Insert a new item.
 */
static int TreeviewInsertCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *parent, *sibling, *newItem;
    Tcl_HashEntry *entryPtr;
    int isNew;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "parent index ?-id id? -options...");
	return TCL_ERROR;
    }

    /* Get parent node:
     */
    if ((parent = FindItem(interp, tv, objv[2])) == NULL) {
	return TCL_ERROR;
    }

    /* Locate previous sibling based on $index:
     */
    if (!strcmp(Tcl_GetString(objv[3]), "end")) {
	sibling = EndPosition(tv, parent);
    } else {
	int index;
	if (Tcl_GetIntFromObj(interp, objv[3], &index) != TCL_OK)
	    return TCL_ERROR;
	sibling = InsertPosition(parent, index);
    }

    /* Get node name:
     *     If -id supplied and does not already exist, use that;
     *     Otherwise autogenerate new one.
     */
    objc -= 4; objv += 4;
    if (objc >= 2 && !strcmp("-id", Tcl_GetString(objv[0]))) {
	const char *itemName = Tcl_GetString(objv[1]);

	entryPtr = Tcl_CreateHashEntry(&tv->tree.items, itemName, &isNew);
	if (!isNew) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Item %s already exists", itemName));
	    Tcl_SetErrorCode(interp, "TTK", "TREE", "ITEM_EXISTS", NULL);
	    return TCL_ERROR;
	}
	objc -= 2; objv += 2;
    } else {
	char idbuf[16];
	do {
	    ++tv->tree.serial;
	    sprintf(idbuf, "I%03X", tv->tree.serial);
	    entryPtr = Tcl_CreateHashEntry(&tv->tree.items, idbuf, &isNew);
	} while (!isNew);
    }

    /* Create and configure new item:
     */
    newItem = NewItem();
    Tk_InitOptions(
	interp, (ClientData)newItem, tv->tree.itemOptionTable, tv->core.tkwin);
    newItem->tagset = Ttk_GetTagSetFromObj(NULL, tv->tree.tagTable, NULL);
    if (ConfigureItem(interp, tv, newItem, objc, objv) != TCL_OK) {
    	Tcl_DeleteHashEntry(entryPtr);
	FreeItem(newItem);
	return TCL_ERROR;
    }

    /* Store in hash table, link into tree:
     */
    Tcl_SetHashValue(entryPtr, newItem);
    newItem->entryPtr = entryPtr;
    InsertItem(parent, sibling, newItem);
    TtkRedisplayWidget(&tv->core);

    Tcl_SetObjResult(interp, ItemID(tv, newItem));
    return TCL_OK;
}

/* + $tv detach $item --
 * 	Unlink $item from the tree.
 */
static int TreeviewDetachCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem **items;
    int i;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item");
	return TCL_ERROR;
    }
    if (!(items = GetItemListFromObj(interp, tv, objv[2]))) {
	return TCL_ERROR;
    }

    /* Sanity-check */
    for (i = 0; items[i]; ++i) {
	if (items[i] == tv->tree.root) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"Cannot detach root item", -1));
	    Tcl_SetErrorCode(interp, "TTK", "TREE", "ROOT", NULL);
	    ckfree(items);
	    return TCL_ERROR;
	}
    }

    for (i = 0; items[i]; ++i) {
	DetachItem(items[i]);
    }

    TtkRedisplayWidget(&tv->core);
    ckfree(items);
    return TCL_OK;
}

/* + $tv delete $items --
 * 	Delete each item in $items.
 *
 * 	Do this in two passes:
 * 	First detach the item and all its descendants and remove them
 * 	from the hash table.  Free the items themselves in a second pass.
 *
 * 	It's done this way because an item may appear more than once
 *	in the list of items to delete (either directly or as a descendant
 *	of a previously deleted item.)
 */

static int TreeviewDeleteCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem **items, *delq;
    int i, selItemDeleted = 0;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "items");
	return TCL_ERROR;
    }

    if (!(items = GetItemListFromObj(interp, tv, objv[2]))) {
	return TCL_ERROR;
    }

    /* Sanity-check:
     */
    for (i=0; items[i]; ++i) {
	if (items[i] == tv->tree.root) {
	    ckfree(items);
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"Cannot delete root item", -1));
	    Tcl_SetErrorCode(interp, "TTK", "TREE", "ROOT", NULL);
	    return TCL_ERROR;
	}
    }

    /* Remove items from hash table.
     */
    delq = 0;
    for (i=0; items[i]; ++i) {
        if (items[i]->state & TTK_STATE_SELECTED) {
            selItemDeleted = 1;
        }
	delq = DeleteItems(items[i], delq);
    }

    /* Free items:
     */
    while (delq) {
	TreeItem *next = delq->next;
	if (tv->tree.focus == delq)
	    tv->tree.focus = 0;
	if (tv->tree.endPtr == delq)
	    tv->tree.endPtr = 0;
	FreeItem(delq);
	delq = next;
    }

    ckfree(items);
    if (selItemDeleted) {
        TtkSendVirtualEvent(tv->core.tkwin, "TreeviewSelect");
    }
    TtkRedisplayWidget(&tv->core);
    return TCL_OK;
}

/* + $tv move $item $parent $index
 * 	Move $item to the specified $index in $parent's child list.
 */
static int TreeviewMoveCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item, *parent;
    TreeItem *sibling;

    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 2, objv, "item parent index");
	return TCL_ERROR;
    }
    if (   (item = FindItem(interp, tv, objv[2])) == 0
	|| (parent = FindItem(interp, tv, objv[3])) == 0)
    {
	return TCL_ERROR;
    }

    /* Locate previous sibling based on $index:
     */
    if (!strcmp(Tcl_GetString(objv[4]), "end")) {
	sibling = EndPosition(tv, parent);
    } else {
	TreeItem *p;
	int index;

	if (Tcl_GetIntFromObj(interp, objv[4], &index) != TCL_OK) {
	    return TCL_ERROR;
	}

	sibling = 0;
	for (p = parent->children; p != NULL && index > 0; p = p->next) {
	    if (p != item) {
		--index;
	    } /* else -- moving node forward, count index+1 nodes  */
	    sibling = p;
	}
    }

    /* Check ancestry:
     */
    if (!AncestryCheck(interp, tv, item, parent)) {
	return TCL_ERROR;
    }

    /* Moving an item after itself is a no-op:
     */
    if (item == sibling) {
	return TCL_OK;
    }

    /* Move item:
     */
    DetachItem(item);
    InsertItem(parent, sibling, item);

    TtkRedisplayWidget(&tv->core);
    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- scrolling
 */

static int TreeviewXViewCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    return TtkScrollviewCommand(interp, objc, objv, tv->tree.xscrollHandle);
}

static int TreeviewYViewCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    return TtkScrollviewCommand(interp, objc, objv, tv->tree.yscrollHandle);
}

/* $tree see $item --
 * 	Ensure that $item is visible.
 */
static int TreeviewSeeCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    TreeItem *item, *parent;
    int rowNumber;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 2, objv, "item");
	return TCL_ERROR;
    }
    if (!(item = FindItem(interp, tv, objv[2]))) {
	return TCL_ERROR;
    }

    /* Make sure all ancestors are open:
     */
    for (parent = item->parent; parent; parent = parent->parent) {
	if (!(parent->state & TTK_STATE_OPEN)) {
	    parent->openObj = unshareObj(parent->openObj);
	    Tcl_SetBooleanObj(parent->openObj, 1);
	    parent->state |= TTK_STATE_OPEN;
	    TtkRedisplayWidget(&tv->core);
	}
    }
    tv->tree.yscroll.total = CountRows(tv->tree.root) - 1;

    /* Make sure item is visible:
     */
    rowNumber = RowNumber(tv, item);
    if (rowNumber < tv->tree.yscroll.first) {
	TtkScrollTo(tv->tree.yscrollHandle, rowNumber, 1);
    } else if (rowNumber >= tv->tree.yscroll.last) {
	TtkScrollTo(tv->tree.yscrollHandle,
	    tv->tree.yscroll.first + (1+rowNumber - tv->tree.yscroll.last), 1);
    }

    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- interactive column resize
 */

/* + $tree drag $column $newX --
 * 	Set right edge of display column $column to x position $X
 */
static int TreeviewDragCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    int left = tv->tree.treeArea.x - tv->tree.xscroll.first;
    int i = FirstColumn(tv);
    TreeColumn *column;
    int newx;

    if (objc != 4) {
	Tcl_WrongNumArgs(interp, 2, objv, "column xposition");
	return TCL_ERROR;
    }

    if (  (column = FindColumn(interp, tv, objv[2])) == 0
        || Tcl_GetIntFromObj(interp, objv[3], &newx) != TCL_OK)
    {
	return TCL_ERROR;
    }

    for (;i < tv->tree.nDisplayColumns; ++i) {
	TreeColumn *c = tv->tree.displayColumns[i];
	int right = left + c->width;
	if (c == column) {
	    DragColumn(tv, i, newx - right);
	    TtkRedisplayWidget(&tv->core);
	    return TCL_OK;
	}
	left = right;
    }

    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	"column %s is not displayed", Tcl_GetString(objv[2])));
    Tcl_SetErrorCode(interp, "TTK", "TREE", "COLUMN_INVISIBLE", NULL);
    return TCL_ERROR;
}

static int TreeviewDropCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "drop");
	return TCL_ERROR;
    }
    ResizeColumns(tv, TreeWidth(tv));
    TtkRedisplayWidget(&tv->core);
    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- focus and selection
 */

/* + $tree focus ?item?
 */
static int TreeviewFocusCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;

    if (objc == 2) {
	if (tv->tree.focus) {
	    Tcl_SetObjResult(interp, ItemID(tv, tv->tree.focus));
	}
	return TCL_OK;
    } else if (objc == 3) {
	TreeItem *newFocus = FindItem(interp, tv, objv[2]);
	if (!newFocus)
	    return TCL_ERROR;
	tv->tree.focus = newFocus;
	TtkRedisplayWidget(&tv->core);
	return TCL_OK;
    } else {
	Tcl_WrongNumArgs(interp, 2, objv, "?newFocus?");
	return TCL_ERROR;
    }
}

/* + $tree selection ?add|remove|set|toggle $items?
 */
static int TreeviewSelectionCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    enum {
	SELECTION_SET, SELECTION_ADD, SELECTION_REMOVE, SELECTION_TOGGLE
    };
    static const char *selopStrings[] = {
	"set", "add", "remove", "toggle", NULL
    };

    Treeview *tv = recordPtr;
    int selop, i;
    TreeItem *item, **items;

    if (objc == 2) {
	Tcl_Obj *result = Tcl_NewListObj(0,0);
	for (item = tv->tree.root->children; item; item=NextPreorder(item)) {
	    if (item->state & TTK_STATE_SELECTED)
		Tcl_ListObjAppendElement(NULL, result, ItemID(tv, item));
	}
	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    }

    if (objc != 4) {
    	Tcl_WrongNumArgs(interp, 2, objv, "?add|remove|set|toggle items?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[2], selopStrings,
	    sizeof(char *), "selection operation", 0, &selop) != TCL_OK) {
	return TCL_ERROR;
    }

    items = GetItemListFromObj(interp, tv, objv[3]);
    if (!items) {
	return TCL_ERROR;
    }

    switch (selop)
    {
	case SELECTION_SET:
	    for (item=tv->tree.root; item; item=NextPreorder(item)) {
		item->state &= ~TTK_STATE_SELECTED;
	    }
	    /*FALLTHRU*/
	case SELECTION_ADD:
	    for (i=0; items[i]; ++i) {
		items[i]->state |= TTK_STATE_SELECTED;
	    }
	    break;
	case SELECTION_REMOVE:
	    for (i=0; items[i]; ++i) {
		items[i]->state &= ~TTK_STATE_SELECTED;
	    }
	    break;
	case SELECTION_TOGGLE:
	    for (i=0; items[i]; ++i) {
		items[i]->state ^= TTK_STATE_SELECTED;
	    }
	    break;
    }

    ckfree(items);
    TtkSendVirtualEvent(tv->core.tkwin, "TreeviewSelect");
    TtkRedisplayWidget(&tv->core);

    return TCL_OK;
}

/*------------------------------------------------------------------------
 * +++ Widget commands -- tags and bindings.
 */

/* + $tv tag bind $tag ?$sequence ?$script??
 */
static int TreeviewTagBindCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    Ttk_TagTable tagTable = tv->tree.tagTable;
    Tk_BindingTable bindingTable = tv->tree.bindingTable;
    Ttk_Tag tag;

    if (objc < 4 || objc > 6) {
    	Tcl_WrongNumArgs(interp, 3, objv, "tagName ?sequence? ?script?");
	return TCL_ERROR;
    }

    tag = Ttk_GetTagFromObj(tagTable, objv[3]);
    if (!tag) { return TCL_ERROR; }

    if (objc == 4) {		/* $tv tag bind $tag */
	Tk_GetAllBindings(interp, bindingTable, tag);
    } else if (objc == 5) { 	/* $tv tag bind $tag $sequence */
	/* TODO: distinguish "no such binding" (OK) from "bad pattern" (ERROR)
	 */
	const char *script = Tk_GetBinding(interp,
		bindingTable, tag, Tcl_GetString(objv[4]));
	if (script != NULL) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(script,-1));
	}
    } else if (objc == 6) {	/* $tv tag bind $tag $sequence $script */
	const char *sequence = Tcl_GetString(objv[4]);
	const char *script = Tcl_GetString(objv[5]);

	if (!*script) { /* Delete existing binding */
	    Tk_DeleteBinding(interp, bindingTable, tag, sequence);
	} else {
	    unsigned long mask = Tk_CreateBinding(interp,
		    bindingTable, tag, sequence, script, 0);

	    /* Test mask to make sure event is supported:
	     */
	    if (mask & (~TreeviewBindEventMask)) {
		Tk_DeleteBinding(interp, bindingTable, tag, sequence);
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "unsupported event %s\nonly key, button, motion, and"
		    " virtual events supported", sequence));
		Tcl_SetErrorCode(interp, "TTK", "TREE", "BIND_EVENTS", NULL);
		return TCL_ERROR;
	    }
	}
    }
    return TCL_OK;
}

/* + $tv tag configure $tag ?-option ?value -option value...??
 */
static int TreeviewTagConfigureCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    Ttk_TagTable tagTable = tv->tree.tagTable;
    Ttk_Tag tag;

    if (objc < 4) {
    	Tcl_WrongNumArgs(interp, 3, objv, "tagName ?-option ?value ...??");
	return TCL_ERROR;
    }

    tag = Ttk_GetTagFromObj(tagTable, objv[3]);

    if (objc == 4) {
	return Ttk_EnumerateTagOptions(interp, tagTable, tag);
    } else if (objc == 5) {
	Tcl_Obj *result = Ttk_TagOptionValue(interp, tagTable, tag, objv[4]);
	if (result) {
	    Tcl_SetObjResult(interp, result);
	    return TCL_OK;
	} /* else */
	return TCL_ERROR;
    }
    /* else */
    TtkRedisplayWidget(&tv->core);
    return Ttk_ConfigureTag(interp, tagTable, tag, objc - 4, objv + 4);
}

/* + $tv tag has $tag ?$item?
 */
static int TreeviewTagHasCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;

    if (objc == 4) {	/* Return list of all items with tag */
	Ttk_Tag tag = Ttk_GetTagFromObj(tv->tree.tagTable, objv[3]);
	TreeItem *item = tv->tree.root;
	Tcl_Obj *result = Tcl_NewListObj(0,0);

	while (item) {
	    if (Ttk_TagSetContains(item->tagset, tag)) {
		Tcl_ListObjAppendElement(NULL, result, ItemID(tv, item));
	    }
	    item = NextPreorder(item);
	}

	Tcl_SetObjResult(interp, result);
	return TCL_OK;
    } else if (objc == 5) {	/* Test if item has specified tag */
	Ttk_Tag tag = Ttk_GetTagFromObj(tv->tree.tagTable, objv[3]);
	TreeItem *item = FindItem(interp, tv, objv[4]);
	if (!item) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp,
	    Tcl_NewBooleanObj(Ttk_TagSetContains(item->tagset, tag)));
	return TCL_OK;
    } else {
    	Tcl_WrongNumArgs(interp, 3, objv, "tagName ?item?");
	return TCL_ERROR;
    }
}

/* + $tv tag names $tag
 */
static int TreeviewTagNamesCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;

    if (objc != 3) {
	Tcl_WrongNumArgs(interp, 3, objv, "");
	return TCL_ERROR;
    }

    return Ttk_EnumerateTags(interp, tv->tree.tagTable);
}

/* + $tv tag add $tag $items
 */
static void AddTag(TreeItem *item, Ttk_Tag tag)
{
    if (Ttk_TagSetAdd(item->tagset, tag)) {
	if (item->tagsObj) Tcl_DecrRefCount(item->tagsObj);
	item->tagsObj = Ttk_NewTagSetObj(item->tagset);
	Tcl_IncrRefCount(item->tagsObj);
    }
}

static int TreeviewTagAddCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    Ttk_Tag tag;
    TreeItem **items;
    int i;

    if (objc != 5) {
	Tcl_WrongNumArgs(interp, 3, objv, "tagName items");
	return TCL_ERROR;
    }

    tag = Ttk_GetTagFromObj(tv->tree.tagTable, objv[3]);
    items = GetItemListFromObj(interp, tv, objv[4]);

    if (!items) {
	return TCL_ERROR;
    }

    for (i=0; items[i]; ++i) {
	AddTag(items[i], tag);
    }

    TtkRedisplayWidget(&tv->core);

    return TCL_OK;
}

/* + $tv tag remove $tag ?$items?
 */
static void RemoveTag(TreeItem *item, Ttk_Tag tag)
{
    if (Ttk_TagSetRemove(item->tagset, tag)) {
	if (item->tagsObj) Tcl_DecrRefCount(item->tagsObj);
	item->tagsObj = Ttk_NewTagSetObj(item->tagset);
	Tcl_IncrRefCount(item->tagsObj);
    }
}

static int TreeviewTagRemoveCommand(
    void *recordPtr, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Treeview *tv = recordPtr;
    Ttk_Tag tag;

    if (objc < 4) {
	Tcl_WrongNumArgs(interp, 3, objv, "tagName items");
	return TCL_ERROR;
    }

    tag = Ttk_GetTagFromObj(tv->tree.tagTable, objv[3]);

    if (objc == 5) {
	TreeItem **items = GetItemListFromObj(interp, tv, objv[4]);
	int i;

	if (!items) {
	    return TCL_ERROR;
	}
	for (i=0; items[i]; ++i) {
	    RemoveTag(items[i], tag);
	}
    } else if (objc == 4) {
	TreeItem *item = tv->tree.root;
	while (item) {
	    RemoveTag(item, tag);
	    item=NextPreorder(item);
	}
    }

    TtkRedisplayWidget(&tv->core);

    return TCL_OK;
}

static const Ttk_Ensemble TreeviewTagCommands[] = {
    { "add",		TreeviewTagAddCommand,0 },
    { "bind",		TreeviewTagBindCommand,0 },
    { "configure",	TreeviewTagConfigureCommand,0 },
    { "has",		TreeviewTagHasCommand,0 },
    { "names",		TreeviewTagNamesCommand,0 },
    { "remove",		TreeviewTagRemoveCommand,0 },
    { 0,0,0 }
};

/*------------------------------------------------------------------------
 * +++ Widget commands record.
 */
static const Ttk_Ensemble TreeviewCommands[] = {
    { "bbox",  		TreeviewBBoxCommand,0 },
    { "children",	TreeviewChildrenCommand,0 },
    { "cget",		TtkWidgetCgetCommand,0 },
    { "column", 	TreeviewColumnCommand,0 },
    { "configure",	TtkWidgetConfigureCommand,0 },
    { "delete", 	TreeviewDeleteCommand,0 },
    { "detach", 	TreeviewDetachCommand,0 },
    { "drag",   	TreeviewDragCommand,0 },
    { "drop",   	TreeviewDropCommand,0 },
    { "exists", 	TreeviewExistsCommand,0 },
    { "focus", 		TreeviewFocusCommand,0 },
    { "heading", 	TreeviewHeadingCommand,0 },
    { "identify",  	TreeviewIdentifyCommand,0 },
    { "index",  	TreeviewIndexCommand,0 },
    { "instate",	TtkWidgetInstateCommand,0 },
    { "insert", 	TreeviewInsertCommand,0 },
    { "item", 		TreeviewItemCommand,0 },
    { "move", 		TreeviewMoveCommand,0 },
    { "next", 		TreeviewNextCommand,0 },
    { "parent", 	TreeviewParentCommand,0 },
    { "prev", 		TreeviewPrevCommand,0 },
    { "see", 		TreeviewSeeCommand,0 },
    { "selection" ,	TreeviewSelectionCommand,0 },
    { "set",  		TreeviewSetCommand,0 },
    { "state",  	TtkWidgetStateCommand,0 },
    { "tag",    	0,TreeviewTagCommands },
    { "xview",  	TreeviewXViewCommand,0 },
    { "yview",  	TreeviewYViewCommand,0 },
    { 0,0,0 }
};

/*------------------------------------------------------------------------
 * +++ Widget definition.
 */

static WidgetSpec TreeviewWidgetSpec = {
    "Treeview",			/* className */
    sizeof(Treeview),   	/* recordSize */
    TreeviewOptionSpecs,	/* optionSpecs */
    TreeviewCommands,   	/* subcommands */
    TreeviewInitialize,   	/* initializeProc */
    TreeviewCleanup,		/* cleanupProc */
    TreeviewConfigure,    	/* configureProc */
    TtkNullPostConfigure,  	/* postConfigureProc */
    TreeviewGetLayout, 		/* getLayoutProc */
    TreeviewSize, 		/* sizeProc */
    TreeviewDoLayout,		/* layoutProc */
    TreeviewDisplay		/* displayProc */
};

/*------------------------------------------------------------------------
 * +++ Layout specifications.
 */

TTK_BEGIN_LAYOUT_TABLE(LayoutTable)

TTK_LAYOUT("Treeview",
    TTK_GROUP("Treeview.field", TTK_FILL_BOTH|TTK_BORDER,
	TTK_GROUP("Treeview.padding", TTK_FILL_BOTH,
	    TTK_NODE("Treeview.treearea", TTK_FILL_BOTH))))

TTK_LAYOUT("Item",
    TTK_GROUP("Treeitem.padding", TTK_FILL_BOTH,
	TTK_NODE("Treeitem.indicator", TTK_PACK_LEFT)
	TTK_NODE("Treeitem.image", TTK_PACK_LEFT)
	TTK_GROUP("Treeitem.focus", TTK_PACK_LEFT,
	    TTK_NODE("Treeitem.text", TTK_PACK_LEFT))))

TTK_LAYOUT("Cell",
    TTK_GROUP("Treedata.padding", TTK_FILL_BOTH,
	TTK_NODE("Treeitem.text", TTK_FILL_BOTH)))

TTK_LAYOUT("Heading",
    TTK_NODE("Treeheading.cell", TTK_FILL_BOTH)
    TTK_GROUP("Treeheading.border", TTK_FILL_BOTH,
	TTK_GROUP("Treeheading.padding", TTK_FILL_BOTH,
	    TTK_NODE("Treeheading.image", TTK_PACK_RIGHT)
	    TTK_NODE("Treeheading.text", TTK_FILL_X))))

TTK_LAYOUT("Row",
    TTK_NODE("Treeitem.row", TTK_FILL_BOTH))

TTK_END_LAYOUT_TABLE

/*------------------------------------------------------------------------
 * +++ Tree indicator element.
 */

typedef struct {
    Tcl_Obj *colorObj;
    Tcl_Obj *sizeObj;
    Tcl_Obj *marginsObj;
} TreeitemIndicator;

static Ttk_ElementOptionSpec TreeitemIndicatorOptions[] = {
    { "-foreground", TK_OPTION_COLOR,
	Tk_Offset(TreeitemIndicator,colorObj), DEFAULT_FOREGROUND },
    { "-indicatorsize", TK_OPTION_PIXELS,
	Tk_Offset(TreeitemIndicator,sizeObj), "12" },
    { "-indicatormargins", TK_OPTION_STRING,
	Tk_Offset(TreeitemIndicator,marginsObj), "2 2 4 2" },
    { NULL, 0, 0, NULL }
};

static void TreeitemIndicatorSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    TreeitemIndicator *indicator = elementRecord;
    Ttk_Padding margins;
    int size = 0;

    Ttk_GetPaddingFromObj(NULL, tkwin, indicator->marginsObj, &margins);
    Tk_GetPixelsFromObj(NULL, tkwin, indicator->sizeObj, &size);

    *widthPtr = size + Ttk_PaddingWidth(margins);
    *heightPtr = size + Ttk_PaddingHeight(margins);
}

static void TreeitemIndicatorDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    TreeitemIndicator *indicator = elementRecord;
    ArrowDirection direction =
	(state & TTK_STATE_OPEN) ? ARROW_DOWN : ARROW_RIGHT;
    Ttk_Padding margins;
    XColor *borderColor = Tk_GetColorFromObj(tkwin, indicator->colorObj);
    XGCValues gcvalues; GC gc; unsigned mask;

    if (state & TTK_STATE_LEAF) /* don't draw anything */
	return;

    Ttk_GetPaddingFromObj(NULL,tkwin,indicator->marginsObj,&margins);
    b = Ttk_PadBox(b, margins);

    gcvalues.foreground = borderColor->pixel;
    gcvalues.line_width = 1;
    mask = GCForeground | GCLineWidth;
    gc = Tk_GetGC(tkwin, mask, &gcvalues);

    TtkDrawArrow(Tk_Display(tkwin), d, gc, b, direction);

    Tk_FreeGC(Tk_Display(tkwin), gc);
}

static Ttk_ElementSpec TreeitemIndicatorElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(TreeitemIndicator),
    TreeitemIndicatorOptions,
    TreeitemIndicatorSize,
    TreeitemIndicatorDraw
};

/*------------------------------------------------------------------------
 * +++ Row element.
 */

typedef struct {
    Tcl_Obj *backgroundObj;
    Tcl_Obj *rowNumberObj;
} RowElement;

static Ttk_ElementOptionSpec RowElementOptions[] = {
    { "-background", TK_OPTION_COLOR,
	Tk_Offset(RowElement,backgroundObj), DEFAULT_BACKGROUND },
    { "-rownumber", TK_OPTION_INT,
	Tk_Offset(RowElement,rowNumberObj), "0" },
    { NULL, 0, 0, NULL }
};

static void RowElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    RowElement *row = elementRecord;
    XColor *color = Tk_GetColorFromObj(tkwin, row->backgroundObj);
    GC gc = Tk_GCForColor(color, d);
    XFillRectangle(Tk_Display(tkwin), d, gc,
	    b.x, b.y, b.width, b.height);
}

static Ttk_ElementSpec RowElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(RowElement),
    RowElementOptions,
    TtkNullElementSize,
    RowElementDraw
};

/*------------------------------------------------------------------------
 * +++ Initialisation.
 */

MODULE_SCOPE
void TtkTreeview_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme = Ttk_GetDefaultTheme(interp);

    RegisterWidget(interp, "ttk::treeview", &TreeviewWidgetSpec);

    Ttk_RegisterElement(interp, theme, "Treeitem.indicator",
	    &TreeitemIndicatorElementSpec, 0);
    Ttk_RegisterElement(interp, theme, "Treeitem.row", &RowElementSpec, 0);
    Ttk_RegisterElement(interp, theme, "Treeheading.cell", &RowElementSpec, 0);
    Ttk_RegisterElement(interp, theme, "treearea", &ttkNullElementSpec, 0);

    Ttk_RegisterLayouts(theme, LayoutTable);
}

/*EOF*/
