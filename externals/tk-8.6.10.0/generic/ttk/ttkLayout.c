/*
 * ttkLayout.c --
 *
 * Generic layout processing.
 *
 * Copyright (c) 2003 Joe English.  Freely redistributable.
 */

#include <string.h>
#include "tkInt.h"
#include "ttkThemeInt.h"

#define MAX(a,b) (a > b ? a : b)
#define MIN(a,b) (a < b ? a : b)

/*------------------------------------------------------------------------
 * +++ Ttk_Box and Ttk_Padding utilities:
 */

Ttk_Box
Ttk_MakeBox(int x, int y, int width, int height)
{
    Ttk_Box b;
    b.x = x; b.y = y; b.width = width; b.height = height;
    return b;
}

int
Ttk_BoxContains(Ttk_Box box, int x, int y)
{
    return box.x <= x && x < box.x + box.width
	&& box.y <= y && y < box.y + box.height;
}

Tcl_Obj *
Ttk_NewBoxObj(Ttk_Box box)
{
    Tcl_Obj *result[4];

    result[0] = Tcl_NewIntObj(box.x);
    result[1] = Tcl_NewIntObj(box.y);
    result[2] = Tcl_NewIntObj(box.width);
    result[3] = Tcl_NewIntObj(box.height);

    return Tcl_NewListObj(4, result);
}

/*
 * packTop, packBottom, packLeft, packRight --
 * 	Carve out a parcel of the specified height (resp width)
 * 	from the specified cavity.
 *
 * Returns:
 * 	The new parcel.
 *
 * Side effects:
 * 	Adjust the cavity.
 */

static Ttk_Box packTop(Ttk_Box *cavity, int height)
{
    Ttk_Box parcel;
    height = MIN(height, cavity->height);
    parcel = Ttk_MakeBox(cavity->x, cavity->y, cavity->width, height);
    cavity->y += height;
    cavity->height -= height;
    return parcel;
}

static Ttk_Box packBottom(Ttk_Box *cavity, int height)
{
    height = MIN(height, cavity->height);
    cavity->height -= height;
    return Ttk_MakeBox(
	cavity->x, cavity->y + cavity->height,
	cavity->width, height);
}

static Ttk_Box packLeft(Ttk_Box *cavity, int width)
{
    Ttk_Box parcel;
    width = MIN(width, cavity->width);
    parcel = Ttk_MakeBox(cavity->x, cavity->y, width,cavity->height);
    cavity->x += width;
    cavity->width -= width;
    return parcel;
}

static Ttk_Box packRight(Ttk_Box *cavity, int width)
{
    width = MIN(width, cavity->width);
    cavity->width -= width;
    return Ttk_MakeBox(cavity->x + cavity->width,
	    cavity->y, width, cavity->height);
}

/*
 * Ttk_PackBox --
 * 	Carve out a parcel of the specified size on the specified side
 * 	in the specified cavity.
 *
 * Returns:
 * 	The new parcel.
 *
 * Side effects:
 * 	Adjust the cavity.
 */

Ttk_Box Ttk_PackBox(Ttk_Box *cavity, int width, int height, Ttk_Side side)
{
    switch (side) {
	default:
	case TTK_SIDE_TOP:	return packTop(cavity, height);
	case TTK_SIDE_BOTTOM:	return packBottom(cavity, height);
	case TTK_SIDE_LEFT:	return packLeft(cavity, width);
	case TTK_SIDE_RIGHT:	return packRight(cavity, width);
    }
}

/*
 * Ttk_PadBox --
 * 	Shrink a box by the specified padding amount.
 */
Ttk_Box Ttk_PadBox(Ttk_Box b, Ttk_Padding p)
{
    b.x += p.left;
    b.y += p.top;
    b.width -= (p.left + p.right);
    b.height -= (p.top + p.bottom);
    if (b.width <= 0) b.width = 1;
    if (b.height <= 0) b.height = 1;
    return b;
}

/*
 * Ttk_ExpandBox --
 * 	Grow a box by the specified padding amount.
 */
Ttk_Box Ttk_ExpandBox(Ttk_Box b, Ttk_Padding p)
{
    b.x -= p.left;
    b.y -= p.top;
    b.width += (p.left + p.right);
    b.height += (p.top + p.bottom);
    return b;
}

/*
 * Ttk_StickBox --
 * 	Place a box of size w * h in the specified parcel,
 * 	according to the specified sticky bits.
 */
Ttk_Box Ttk_StickBox(Ttk_Box parcel, int width, int height, unsigned sticky)
{
    int dx, dy;

    if (width > parcel.width) width = parcel.width;
    if (height > parcel.height) height = parcel.height;

    dx = parcel.width - width;
    dy = parcel.height - height;

    /*
     * X coordinate adjustment:
     */
    switch (sticky & (TTK_STICK_W | TTK_STICK_E))
    {
	case TTK_STICK_W | TTK_STICK_E:
	    /* no-op -- use entire parcel width */
	    break;
	case TTK_STICK_W:
	    parcel.width = width;
	    break;
	case TTK_STICK_E:
	    parcel.x += dx;
	    parcel.width = width;
	    break;
	default :
	    parcel.x += dx / 2;
	    parcel.width = width;
	    break;
    }

    /*
     * Y coordinate adjustment:
     */
    switch (sticky & (TTK_STICK_N | TTK_STICK_S))
    {
	case TTK_STICK_N | TTK_STICK_S:
	    /* use entire parcel height */
	    break;
	case TTK_STICK_N:
	    parcel.height = height;
	    break;
	case TTK_STICK_S:
	    parcel.y += dy;
	    parcel.height = height;
	    break;
	default :
	    parcel.y += dy / 2;
	    parcel.height = height;
	    break;
    }

    return parcel;
}

/*
 * AnchorToSticky --
 * 	Convert a Tk_Anchor enum to a TTK_STICKY bitmask.
 */
static Ttk_Sticky AnchorToSticky(Tk_Anchor anchor)
{
    switch (anchor)
    {
	case TK_ANCHOR_N:	return TTK_STICK_N;
	case TK_ANCHOR_NE:	return TTK_STICK_N | TTK_STICK_E;
	case TK_ANCHOR_E:	return TTK_STICK_E;
	case TK_ANCHOR_SE:	return TTK_STICK_S | TTK_STICK_E;
	case TK_ANCHOR_S:	return TTK_STICK_S;
	case TK_ANCHOR_SW:	return TTK_STICK_S | TTK_STICK_W;
	case TK_ANCHOR_W:	return TTK_STICK_W;
	case TK_ANCHOR_NW:	return TTK_STICK_N | TTK_STICK_W;
	default:
	case TK_ANCHOR_CENTER:	return 0;
    }
}

/*
 * Ttk_AnchorBox --
 * 	Place a box of size w * h in the specified parcel,
 * 	according to the specified anchor.
 */
Ttk_Box Ttk_AnchorBox(Ttk_Box parcel, int width, int height, Tk_Anchor anchor)
{
    return Ttk_StickBox(parcel, width, height, AnchorToSticky(anchor));
}

/*
 * Ttk_PlaceBox --
 * 	Combine Ttk_PackBox() and Ttk_StickBox().
 */
Ttk_Box Ttk_PlaceBox(
    Ttk_Box *cavity, int width, int height, Ttk_Side side, unsigned sticky)
{
    return Ttk_StickBox(
	    Ttk_PackBox(cavity, width, height, side), width, height, sticky);
}

/*
 * Ttk_PositionBox --
 * 	Pack and stick a box according to PositionSpec flags.
 */
MODULE_SCOPE Ttk_Box
Ttk_PositionBox(Ttk_Box *cavity, int width, int height, Ttk_PositionSpec flags)
{
    Ttk_Box parcel;

	 if (flags & TTK_EXPAND)	parcel = *cavity;
    else if (flags & TTK_PACK_TOP)	parcel = packTop(cavity, height);
    else if (flags & TTK_PACK_LEFT)	parcel = packLeft(cavity, width);
    else if (flags & TTK_PACK_BOTTOM)	parcel = packBottom(cavity, height);
    else if (flags & TTK_PACK_RIGHT)	parcel = packRight(cavity, width);
    else				parcel = *cavity;

    return Ttk_StickBox(parcel, width, height, flags);
}

/*
 * TTKInitPadding --
 * 	Common factor of Ttk_GetPaddingFromObj and Ttk_GetBorderFromObj.
 * 	Initializes Ttk_Padding record, supplying default values
 * 	for missing entries.
 */
static void TTKInitPadding(int padc, int pixels[4], Ttk_Padding *pad)
{
    switch (padc)
    {
	case 0: pixels[0] = 0; /*FALLTHRU*/
	case 1:	pixels[1] = pixels[0]; /*FALLTHRU*/
	case 2:	pixels[2] = pixels[0]; /*FALLTHRU*/
	case 3:	pixels[3] = pixels[1]; /*FALLTHRU*/
    }

    pad->left	= (short)pixels[0];
    pad->top	= (short)pixels[1];
    pad->right	= (short)pixels[2];
    pad->bottom	= (short)pixels[3];
}

/*
 * Ttk_GetPaddingFromObj --
 *
 * 	Extract a padding specification from a Tcl_Obj * scaled
 * 	to work with a particular Tk_Window.
 *
 * 	The string representation of a Ttk_Padding is a list
 * 	of one to four Tk_Pixel specifications, corresponding
 * 	to the left, top, right, and bottom padding.
 *
 * 	If the 'bottom' (fourth) element is missing, it defaults to 'top'.
 * 	If the 'right' (third) element is missing, it defaults to 'left'.
 * 	If the 'top' (second) element is missing, it defaults to 'left'.
 *
 * 	The internal representation is a Tcl_ListObj containing
 * 	one to four Tk_PixelObj objects.
 *
 * Returns:
 * 	TCL_OK or TCL_ERROR.  In the latter case an error message is
 * 	left in 'interp' and '*paddingPtr' is set to all-zeros.
 * 	Otherwise, *paddingPtr is filled in with the padding specification.
 *
 */
int Ttk_GetPaddingFromObj(
    Tcl_Interp *interp,
    Tk_Window tkwin,
    Tcl_Obj *objPtr,
    Ttk_Padding *pad)
{
    Tcl_Obj **padv;
    int i, padc, pixels[4];

    if (TCL_OK != Tcl_ListObjGetElements(interp, objPtr, &padc, &padv)) {
	goto error;
    }

    if (padc > 4) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "Wrong #elements in padding spec", -1));
	    Tcl_SetErrorCode(interp, "TTK", "VALUE", "PADDING", NULL);
	}
	goto error;
    }

    for (i=0; i < padc; ++i) {
	if (Tk_GetPixelsFromObj(interp, tkwin, padv[i], &pixels[i]) != TCL_OK) {
	    goto error;
	}
    }

    TTKInitPadding(padc, pixels, pad);
    return TCL_OK;

error:
    pad->left = pad->top = pad->right = pad->bottom = 0;
    return TCL_ERROR;
}

/* Ttk_GetBorderFromObj --
 * 	Same as Ttk_GetPaddingFromObj, except padding is a list of integers
 * 	instead of Tk_Pixel specifications.  Does not require a Tk_Window
 * 	parameter.
 *
 */
int Ttk_GetBorderFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Ttk_Padding *pad)
{
    Tcl_Obj **padv;
    int i, padc, pixels[4];

    if (TCL_OK != Tcl_ListObjGetElements(interp, objPtr, &padc, &padv)) {
	goto error;
    }

    if (padc > 4) {
	if (interp) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "Wrong #elements in padding spec", -1));
	    Tcl_SetErrorCode(interp, "TTK", "VALUE", "BORDER", NULL);
	}
	goto error;
    }

    for (i=0; i < padc; ++i) {
	if (Tcl_GetIntFromObj(interp, padv[i], &pixels[i]) != TCL_OK) {
	    goto error;
	}
    }

    TTKInitPadding(padc, pixels, pad);
    return TCL_OK;

error:
    pad->left = pad->top = pad->right = pad->bottom = 0;
    return TCL_ERROR;
}

/*
 * Ttk_MakePadding --
 *	Return an initialized Ttk_Padding structure.
 */
Ttk_Padding Ttk_MakePadding(short left, short top, short right, short bottom)
{
    Ttk_Padding pad;
    pad.left = left;
    pad.top = top;
    pad.right = right;
    pad.bottom = bottom;
    return pad;
}

/*
 * Ttk_UniformPadding --
 * 	Returns a uniform Ttk_Padding structure, with the same
 * 	border width on all sides.
 */
Ttk_Padding Ttk_UniformPadding(short borderWidth)
{
    Ttk_Padding pad;
    pad.left = pad.top = pad.right = pad.bottom = borderWidth;
    return pad;
}

/*
 * Ttk_AddPadding --
 *	Combine two padding records.
 */
Ttk_Padding Ttk_AddPadding(Ttk_Padding p1, Ttk_Padding p2)
{
    p1.left += p2.left;
    p1.top += p2.top;
    p1.right += p2.right;
    p1.bottom += p2.bottom;
    return p1;
}

/* Ttk_RelievePadding --
 * 	Add an extra n pixels of padding according to specified relief.
 * 	This may be used in element geometry procedures to simulate
 * 	a "pressed-in" look for pushbuttons.
 */
Ttk_Padding Ttk_RelievePadding(Ttk_Padding padding, int relief, int n)
{
    switch (relief)
    {
	case TK_RELIEF_RAISED:
	    padding.right += n;
	    padding.bottom += n;
	    break;
	case TK_RELIEF_SUNKEN:	/* shift */
	    padding.left += n;
	    padding.top += n;
	    break;
	default:
	{
	    int h1 = n/2, h2 = h1 + n % 2;
	    padding.left += h1;
	    padding.top += h1;
	    padding.right += h2;
	    padding.bottom += h2;
	    break;
	}
    }
    return padding;
}

/*
 * Ttk_GetStickyFromObj --
 * 	Returns a stickiness specification from the specified Tcl_Obj*,
 * 	consisting of any combination of n, s, e, and w.
 *
 * Returns: TCL_OK if objPtr holds a valid stickiness specification,
 *	otherwise TCL_ERROR.  interp is used for error reporting if non-NULL.
 *
 */
int Ttk_GetStickyFromObj(
    Tcl_Interp *interp, Tcl_Obj *objPtr, Ttk_Sticky *result)
{
    const char *string = Tcl_GetString(objPtr);
    Ttk_Sticky sticky = 0;
    char c;

    while ((c = *string++) != '\0') {
	switch (c) {
	    case 'w': case 'W': sticky |= TTK_STICK_W; break;
	    case 'e': case 'E': sticky |= TTK_STICK_E; break;
	    case 'n': case 'N': sticky |= TTK_STICK_N; break;
	    case 's': case 'S': sticky |= TTK_STICK_S; break;
	    default:
	    	if (interp) {
		    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"Bad -sticky specification %s",
			Tcl_GetString(objPtr)));
		    Tcl_SetErrorCode(interp, "TTK", "VALUE", "STICKY", NULL);
		}
		return TCL_ERROR;
	}
    }

    *result = sticky;
    return TCL_OK;
}

/* Ttk_NewStickyObj --
 * 	Construct a new Tcl_Obj * containing a stickiness specification.
 */
Tcl_Obj *Ttk_NewStickyObj(Ttk_Sticky sticky)
{
    char buf[5];
    char *p = buf;

    if (sticky & TTK_STICK_N)	*p++ = 'n';
    if (sticky & TTK_STICK_S)	*p++ = 's';
    if (sticky & TTK_STICK_W)	*p++ = 'w';
    if (sticky & TTK_STICK_E)	*p++ = 'e';

    *p = '\0';
    return Tcl_NewStringObj(buf, p - buf);
}

/*------------------------------------------------------------------------
 * +++ Layout nodes.
 */

typedef struct Ttk_LayoutNode_ Ttk_LayoutNode;
struct Ttk_LayoutNode_
{
    unsigned		flags;		/* Packing and sticky flags */
    Ttk_ElementClass 	*eclass;	/* Class record */
    Ttk_State 	 	state;		/* Current state */
    Ttk_Box 		parcel;		/* allocated parcel */
    Ttk_LayoutNode	*next, *child;
};

static Ttk_LayoutNode *Ttk_NewLayoutNode(
    unsigned flags, Ttk_ElementClass *elementClass)
{
    Ttk_LayoutNode *node = ckalloc(sizeof(*node));

    node->flags = flags;
    node->eclass = elementClass;
    node->state = 0u;
    node->next = node->child = 0;
    node->parcel = Ttk_MakeBox(0,0,0,0);

    return node;
}

static void Ttk_FreeLayoutNode(Ttk_LayoutNode *node)
{
    while (node) {
	Ttk_LayoutNode *next = node->next;
	Ttk_FreeLayoutNode(node->child);
	ckfree(node);
	node = next;
    }
}

/*------------------------------------------------------------------------
 * +++ Layout templates.
 */

struct Ttk_TemplateNode_ {
    char *name;
    unsigned flags;
    struct Ttk_TemplateNode_ *next, *child;
};

static Ttk_TemplateNode *Ttk_NewTemplateNode(const char *name, unsigned flags)
{
    Ttk_TemplateNode *op = ckalloc(sizeof(*op));
    op->name = ckalloc(strlen(name) + 1); strcpy(op->name, name);
    op->flags = flags;
    op->next = op->child = 0;
    return op;
}

void Ttk_FreeLayoutTemplate(Ttk_LayoutTemplate op)
{
    while (op) {
	Ttk_LayoutTemplate next = op->next;
	Ttk_FreeLayoutTemplate(op->child);
	ckfree(op->name);
	ckfree(op);
	op = next;
    }
}

/* InstantiateLayout --
 *	Create a layout tree from a template.
 */
static Ttk_LayoutNode *
Ttk_InstantiateLayout(Ttk_Theme theme, Ttk_TemplateNode *op)
{
    Ttk_ElementClass *elementClass = Ttk_GetElement(theme, op->name);
    Ttk_LayoutNode *node = Ttk_NewLayoutNode(op->flags, elementClass);

    if (op->next) {
	node->next = Ttk_InstantiateLayout(theme,op->next);
    }
    if (op->child) {
	node->child = Ttk_InstantiateLayout(theme,op->child);
    }

    return node;
}

/*
 * Ttk_ParseLayoutTemplate --
 *	Convert a Tcl list into a layout template.
 *
 * Syntax:
 * 	layoutSpec ::= { elementName ?-option value ...? }+
 */

/* NB: This must match bit definitions TTK_PACK_LEFT etc. */
static const char *packSideStrings[] =
    { "left", "right", "top", "bottom", NULL };

Ttk_LayoutTemplate Ttk_ParseLayoutTemplate(Tcl_Interp *interp, Tcl_Obj *objPtr)
{
    enum {  OP_SIDE, OP_STICKY, OP_EXPAND, OP_BORDER, OP_UNIT, OP_CHILDREN };
    static const char *optStrings[] = {
	"-side", "-sticky", "-expand", "-border", "-unit", "-children", 0 };

    int i = 0, objc;
    Tcl_Obj **objv;
    Ttk_TemplateNode *head = 0, *tail = 0;

    if (Tcl_ListObjGetElements(interp, objPtr, &objc, &objv) != TCL_OK)
	return 0;

    while (i < objc) {
	const char *elementName = Tcl_GetString(objv[i]);
	unsigned flags = 0x0, sticky = TTK_FILL_BOTH;
	Tcl_Obj *childSpec = 0;

	/*
	 * Parse options:
	 */
	++i;
	while (i < objc) {
	    const char *optName = Tcl_GetString(objv[i]);
	    int option, value;

	    if (optName[0] != '-')
		break;

	    if (Tcl_GetIndexFromObjStruct(interp, objv[i], optStrings,
		    sizeof(char *), "option", 0, &option)
		!= TCL_OK)
	    {
		goto error;
	    }

	    if (++i >= objc) {
		Tcl_SetObjResult(interp, Tcl_ObjPrintf(
			"Missing value for option %s",
			Tcl_GetString(objv[i-1])));
		Tcl_SetErrorCode(interp, "TTK", "VALUE", "LAYOUT", NULL);
		goto error;
	    }

	    switch (option) {
		case OP_SIDE:	/* <<NOTE-PACKSIDE>> */
		    if (Tcl_GetIndexFromObjStruct(interp, objv[i], packSideStrings,
				sizeof(char *), "side", 0, &value) != TCL_OK)
		    {
			goto error;
		    }
		    flags |= (TTK_PACK_LEFT << value);

		    break;
		case OP_STICKY:
		    if (Ttk_GetStickyFromObj(interp,objv[i],&sticky) != TCL_OK)
			goto error;
		    break;
		case OP_EXPAND:
		    if (Tcl_GetBooleanFromObj(interp,objv[i],&value) != TCL_OK)
			goto error;
		    if (value)
			flags |= TTK_EXPAND;
		    break;
		case OP_BORDER:
		    if (Tcl_GetBooleanFromObj(interp,objv[i],&value) != TCL_OK)
			goto error;
		    if (value)
			flags |= TTK_BORDER;
		    break;
		case OP_UNIT:
		    if (Tcl_GetBooleanFromObj(interp,objv[i],&value) != TCL_OK)
			goto error;
		    if (value)
			flags |= TTK_UNIT;
		    break;
		case OP_CHILDREN:
		    childSpec = objv[i];
		    break;
	    }
	    ++i;
	}

	/*
	 * Build new node:
	 */
	if (tail) {
	    tail->next = Ttk_NewTemplateNode(elementName, flags | sticky);
	    tail = tail->next;
	} else {
	    head = tail = Ttk_NewTemplateNode(elementName, flags | sticky);
	}
	if (childSpec) {
	    tail->child = Ttk_ParseLayoutTemplate(interp, childSpec);
	    if (!tail->child) {
                Tcl_SetObjResult(interp, Tcl_ObjPrintf("Invalid -children value"));
                Tcl_SetErrorCode(interp, "TTK", "VALUE", "CHILDREN", NULL);
		goto error;
	    }
	}
    }

    return head;

error:
    Ttk_FreeLayoutTemplate(head);
    return 0;
}

/* Ttk_BuildLayoutTemplate --
 * 	Build a layout template tree from a statically defined
 * 	Ttk_LayoutSpec array.
 */
Ttk_LayoutTemplate Ttk_BuildLayoutTemplate(Ttk_LayoutSpec spec)
{
    Ttk_TemplateNode *first = 0, *last = 0;

    for ( ; !(spec->opcode & _TTK_LAYOUT_END) ; ++spec) {
	if (spec->elementName) {
	    Ttk_TemplateNode *node =
		Ttk_NewTemplateNode(spec->elementName, spec->opcode);

	    if (last) {
		last->next = node;
	    } else {
		first = node;
	    }
	    last = node;
	}

	if (spec->opcode & _TTK_CHILDREN && last) {
	    int depth = 1;
	    last->child = Ttk_BuildLayoutTemplate(spec+1);

	    /* Skip to end of group:
	     */
	    while (depth) {
		++spec;
		if (spec->opcode & _TTK_CHILDREN) {
		    ++depth;
		}
		if (spec->opcode & _TTK_LAYOUT_END) {
		    --depth;
		}
	    }
	}

    } /* for */

    return first;
}

void Ttk_RegisterLayouts(Ttk_Theme theme, Ttk_LayoutSpec spec)
{
    while (!(spec->opcode & _TTK_LAYOUT_END)) {
	Ttk_LayoutTemplate layoutTemplate = Ttk_BuildLayoutTemplate(spec+1);
	Ttk_RegisterLayoutTemplate(theme, spec->elementName, layoutTemplate);
	do {
	    ++spec;
	} while (!(spec->opcode & _TTK_LAYOUT));
    }
}

Tcl_Obj *Ttk_UnparseLayoutTemplate(Ttk_TemplateNode *node)
{
    Tcl_Obj *result = Tcl_NewListObj(0,0);

#   define APPENDOBJ(obj) Tcl_ListObjAppendElement(NULL, result, obj)
#   define APPENDSTR(str) APPENDOBJ(Tcl_NewStringObj(str,-1))

    while (node) {
	unsigned flags = node->flags;

	APPENDSTR(node->name);

	/* Back-compute -side.  <<NOTE-PACKSIDE>>
	 * @@@ NOTES: Ick.
	 */
	if (flags & TTK_EXPAND) {
	    APPENDSTR("-expand");
	    APPENDSTR("1");
	} else {
	    if (flags & _TTK_MASK_PACK) {
		int side = 0;
		unsigned sideFlags = flags & _TTK_MASK_PACK;

		while (!(sideFlags & TTK_PACK_LEFT)) {
		    ++side;
		    sideFlags >>= 1;
		}
		APPENDSTR("-side");
		APPENDSTR(packSideStrings[side]);
	    }
	}

	/*
	 * In Ttk_ParseLayoutTemplate, default -sticky is "nsew", so always
	 * include this even if no sticky bits are set.
	 */

	APPENDSTR("-sticky");
	APPENDOBJ(Ttk_NewStickyObj(flags & _TTK_MASK_STICK));

	/* @@@ Check again: are these necessary? */
	if (flags & TTK_BORDER)	{ APPENDSTR("-border"); APPENDSTR("1"); }
	if (flags & TTK_UNIT) 	{ APPENDSTR("-unit"); APPENDSTR("1"); }

	if (node->child) {
	    APPENDSTR("-children");
	    APPENDOBJ(Ttk_UnparseLayoutTemplate(node->child));
	}
	node = node->next;
    }

#   undef APPENDOBJ
#   undef APPENDSTR

    return result;
}

/*------------------------------------------------------------------------
 * +++ Layouts.
 */
struct Ttk_Layout_
{
    Ttk_Style	 	style;
    void 		*recordPtr;
    Tk_OptionTable	optionTable;
    Tk_Window		tkwin;
    Ttk_LayoutNode	*root;
};

static Ttk_Layout TTKNewLayout(
    Ttk_Style style,
    void *recordPtr,Tk_OptionTable optionTable, Tk_Window tkwin,
    Ttk_LayoutNode *root)
{
    Ttk_Layout layout = ckalloc(sizeof(*layout));
    layout->style = style;
    layout->recordPtr = recordPtr;
    layout->optionTable = optionTable;
    layout->tkwin = tkwin;
    layout->root = root;
    return layout;
}

void Ttk_FreeLayout(Ttk_Layout layout)
{
    Ttk_FreeLayoutNode(layout->root);
    ckfree(layout);
}

/*
 * Ttk_CreateLayout --
 *	Create a layout from the specified theme and style name.
 *	Returns: New layout, 0 on error.
 *	Leaves an error message in interp's result if there is an error.
 */
Ttk_Layout Ttk_CreateLayout(
    Tcl_Interp *interp,		/* where to leave error messages */
    Ttk_Theme themePtr,
    const char *styleName,
    void *recordPtr,
    Tk_OptionTable optionTable,
    Tk_Window tkwin)
{
    Ttk_Style style = Ttk_GetStyle(themePtr, styleName);
    Ttk_LayoutTemplate layoutTemplate =
	Ttk_FindLayoutTemplate(themePtr,styleName);
    Ttk_ElementClass *bgelement = Ttk_GetElement(themePtr, "background");
    Ttk_LayoutNode *bgnode;

    if (!layoutTemplate) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Layout %s not found", styleName));
	Tcl_SetErrorCode(interp, "TTK", "LOOKUP", "LAYOUT", styleName, NULL);
	return 0;
    }

    bgnode = Ttk_NewLayoutNode(TTK_FILL_BOTH, bgelement);
    bgnode->next = Ttk_InstantiateLayout(themePtr, layoutTemplate);

    return TTKNewLayout(style, recordPtr, optionTable, tkwin, bgnode);
}

/* Ttk_CreateSublayout --
 * 	Creates a new sublayout.
 *
 * 	Sublayouts are used to draw subparts of a compound widget.
 *	They use the same Tk_Window, but a different option table
 *	and data record.
 */
Ttk_Layout
Ttk_CreateSublayout(
    Tcl_Interp *interp,
    Ttk_Theme themePtr,
    Ttk_Layout parentLayout,
    const char *baseName,
    Tk_OptionTable optionTable)
{
    Tcl_DString buf;
    const char *styleName;
    Ttk_Style style;
    Ttk_LayoutTemplate layoutTemplate;

    Tcl_DStringInit(&buf);
    Tcl_DStringAppend(&buf, Ttk_StyleName(parentLayout->style), -1);
    Tcl_DStringAppend(&buf, baseName, -1);
    styleName = Tcl_DStringValue(&buf);

    style = Ttk_GetStyle(themePtr, styleName);
    layoutTemplate = Ttk_FindLayoutTemplate(themePtr, styleName);

    if (!layoutTemplate) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"Layout %s not found", styleName));
	Tcl_SetErrorCode(interp, "TTK", "LOOKUP", "LAYOUT", styleName, NULL);
	return 0;
    }

    Tcl_DStringFree(&buf);

    return TTKNewLayout(
	    style, 0, optionTable, parentLayout->tkwin,
	    Ttk_InstantiateLayout(themePtr, layoutTemplate));
}

/* Ttk_RebindSublayout --
 * 	Bind sublayout to new data source.
 */
void Ttk_RebindSublayout(Ttk_Layout layout, void *recordPtr)
{
    layout->recordPtr = recordPtr;
}

/*
 * Ttk_QueryOption --
 * 	Look up an option from a layout's associated option.
 */
Tcl_Obj *Ttk_QueryOption(
    Ttk_Layout layout, const char *optionName, Ttk_State state)
{
    return Ttk_QueryStyle(
	layout->style,layout->recordPtr,layout->optionTable,optionName,state);
}

/*
 * Ttk_LayoutStyle --
 * 	Extract Ttk_Style from Ttk_Layout.
 */
Ttk_Style Ttk_LayoutStyle(Ttk_Layout layout)
{
    return layout->style;
}

/*------------------------------------------------------------------------
 * +++ Size computation.
 */
static void Ttk_NodeListSize(
    Ttk_Layout layout, Ttk_LayoutNode *node,
    Ttk_State state, int *widthPtr, int *heightPtr); /* Forward */

static void Ttk_NodeSize(
    Ttk_Layout layout, Ttk_LayoutNode *node, Ttk_State state,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    int elementWidth, elementHeight, subWidth, subHeight;
    Ttk_Padding elementPadding;

    Ttk_ElementSize(node->eclass,
	layout->style, layout->recordPtr,layout->optionTable, layout->tkwin,
	state|node->state,
	&elementWidth, &elementHeight, &elementPadding);

    Ttk_NodeListSize(layout,node->child,state,&subWidth,&subHeight);
    subWidth += Ttk_PaddingWidth(elementPadding);
    subHeight += Ttk_PaddingHeight(elementPadding);

    *widthPtr = MAX(elementWidth, subWidth);
    *heightPtr = MAX(elementHeight, subHeight);
    *paddingPtr = elementPadding;
}

static void Ttk_NodeListSize(
    Ttk_Layout layout, Ttk_LayoutNode *node,
    Ttk_State state, int *widthPtr, int *heightPtr)
{
    if (!node) {
	*widthPtr = *heightPtr = 0;
    } else {
	int width, height, restWidth, restHeight;
	Ttk_Padding unused;

	Ttk_NodeSize(layout, node, state, &width, &height, &unused);
	Ttk_NodeListSize(layout, node->next, state, &restWidth, &restHeight);

	if (node->flags & (TTK_PACK_LEFT|TTK_PACK_RIGHT)) {
	    *widthPtr = width + restWidth;
	} else {
	    *widthPtr = MAX(width, restWidth);
	}

	if (node->flags & (TTK_PACK_TOP|TTK_PACK_BOTTOM)) {
	    *heightPtr = height + restHeight;
	} else {
	    *heightPtr = MAX(height, restHeight);
	}
    }
}

/*
 * Ttk_LayoutNodeInternalPadding --
 * 	Returns the internal padding of a layout node.
 */
Ttk_Padding Ttk_LayoutNodeInternalPadding(
    Ttk_Layout layout, Ttk_LayoutNode *node)
{
    int unused;
    Ttk_Padding padding;
    Ttk_ElementSize(node->eclass,
	layout->style, layout->recordPtr, layout->optionTable, layout->tkwin,
	0/*state*/, &unused, &unused, &padding);
    return padding;
}

/*
 * Ttk_LayoutNodeInternalParcel --
 * 	Returns the inner area of a specified layout node,
 * 	based on current parcel and element's internal padding.
 */
Ttk_Box Ttk_LayoutNodeInternalParcel(Ttk_Layout layout, Ttk_LayoutNode *node)
{
    Ttk_Padding padding = Ttk_LayoutNodeInternalPadding(layout, node);
    return Ttk_PadBox(node->parcel, padding);
}

/* Ttk_LayoutSize --
 * 	Compute requested size of a layout.
 */
void Ttk_LayoutSize(
    Ttk_Layout layout, Ttk_State state, int *widthPtr, int *heightPtr)
{
    Ttk_NodeListSize(layout, layout->root, state, widthPtr, heightPtr);
}

void Ttk_LayoutNodeReqSize(	/* @@@ Rename this */
    Ttk_Layout layout, Ttk_LayoutNode *node, int *widthPtr, int *heightPtr)
{
    Ttk_Padding unused;
    Ttk_NodeSize(layout, node, 0/*state*/, widthPtr, heightPtr, &unused);
}

/*------------------------------------------------------------------------
 * +++ Layout placement.
 */

/* Ttk_PlaceNodeList --
 *	Compute parcel for each node in a layout tree
 *	according to position specification and overall size.
 */
static void Ttk_PlaceNodeList(
    Ttk_Layout layout, Ttk_LayoutNode *node, Ttk_State state, Ttk_Box cavity)
{
    for (; node; node = node->next)
    {
	int width, height;
	Ttk_Padding padding;

	/* Compute node size: (@@@ cache this instead?)
	 */
	Ttk_NodeSize(layout, node, state, &width, &height, &padding);

	/* Compute parcel:
	 */
	node->parcel = Ttk_PositionBox(&cavity, width, height, node->flags);

	/* Place child nodes:
	 */
	if (node->child) {
	    Ttk_Box childBox = Ttk_PadBox(node->parcel, padding);
	    Ttk_PlaceNodeList(layout,node->child, state, childBox);
	}
    }
}

void Ttk_PlaceLayout(Ttk_Layout layout, Ttk_State state, Ttk_Box b)
{
    Ttk_PlaceNodeList(layout, layout->root, state,  b);
}

/*------------------------------------------------------------------------
 * +++ Layout drawing.
 */

/*
 * Ttk_DrawLayout --
 * 	Draw a layout tree.
 */
static void Ttk_DrawNodeList(
    Ttk_Layout layout, Ttk_State state, Ttk_LayoutNode *node, Drawable d)
{
    for (; node; node = node->next)
    {
	int border = node->flags & TTK_BORDER;
	int substate = state;

	if (node->flags & TTK_UNIT)
	    substate |= node->state;

	if (node->child && border)
	    Ttk_DrawNodeList(layout, substate, node->child, d);

	Ttk_DrawElement(
	    node->eclass,
	    layout->style,layout->recordPtr,layout->optionTable,layout->tkwin,
	    d, node->parcel, state | node->state);

	if (node->child && !border)
	    Ttk_DrawNodeList(layout, substate, node->child, d);
    }
}

void Ttk_DrawLayout(Ttk_Layout layout, Ttk_State state, Drawable d)
{
    Ttk_DrawNodeList(layout, state, layout->root, d);
}

/*------------------------------------------------------------------------
 * +++ Inquiry and modification.
 */

/*
 * Ttk_IdentifyElement --
 * 	Find the element at the specified x,y coordinate.
 */
static Ttk_Element IdentifyNode(Ttk_Element node, int x, int y)
{
    Ttk_Element closest = NULL;

    for (; node; node = node->next) {
	if (Ttk_BoxContains(node->parcel, x, y)) {
	    closest = node;
	    if (node->child && !(node->flags & TTK_UNIT)) {
		Ttk_Element childNode = IdentifyNode(node->child, x,y);
		if (childNode) {
		    closest = childNode;
		}
	    }
	}
    }
    return closest;
}

Ttk_Element Ttk_IdentifyElement(Ttk_Layout layout, int x, int y)
{
    return IdentifyNode(layout->root, x, y);
}

/*
 * tail --
 * 	Return the last component of an element name, e.g.,
 * 	"Scrollbar.thumb" => "thumb"
 */
static const char *tail(const char *elementName)
{
    const char *dot;
    while ((dot=strchr(elementName,'.')) != NULL)
	elementName = dot + 1;
    return elementName;
}

/*
 * Ttk_FindElement --
 * 	Look up an element by name
 */
static Ttk_Element
FindNode(Ttk_Element node, const char *nodeName)
{
    for (; node ; node = node->next) {
	if (!strcmp(tail(Ttk_ElementName(node)), nodeName))
	    return node;

	if (node->child) {
	    Ttk_Element childNode = FindNode(node->child, nodeName);
	    if (childNode)
		return childNode;
	}
    }
    return 0;
}

Ttk_Element Ttk_FindElement(Ttk_Layout layout, const char *nodeName)
{
    return FindNode(layout->root, nodeName);
}

/*
 * Ttk_ClientRegion --
 * 	Find the internal parcel of a named element within a given layout.
 * 	If the element is not present, use the entire window.
 */
Ttk_Box Ttk_ClientRegion(Ttk_Layout layout, const char *elementName)
{
    Ttk_Element element = Ttk_FindElement(layout, elementName);
    return element
	? Ttk_LayoutNodeInternalParcel(layout, element)
	: Ttk_WinBox(layout->tkwin)
	;
}

/*
 * Ttk_ElementName --
 * 	Return the name (class name) of the element.
 */
const char *Ttk_ElementName(Ttk_Element node)
{
    return Ttk_ElementClassName(node->eclass);
}

/*
 * Ttk_ElementParcel --
 * 	Return the element's current parcel.
 */
Ttk_Box Ttk_ElementParcel(Ttk_Element node)
{
    return node->parcel;
}

/*
 * Ttk_PlaceElement --
 * 	Explicitly specify an element's parcel.
 */
void Ttk_PlaceElement(Ttk_Layout layout, Ttk_Element node, Ttk_Box b)
{
    node->parcel = b;
    if (node->child) {
	Ttk_PlaceNodeList(layout, node->child, 0,
	    Ttk_PadBox(b, Ttk_LayoutNodeInternalPadding(layout, node)));
    }
}

/*
 * Ttk_ChangeElementState --
 */
void Ttk_ChangeElementState(Ttk_LayoutNode *node,unsigned set,unsigned clr)
{
    node->state = (node->state | set) & ~clr;
}

/*EOF*/
