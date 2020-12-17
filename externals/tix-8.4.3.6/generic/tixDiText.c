/*
 * tixDiText.c --
 *
 *	This file implements one of the "Display Items" in the Tix library :
 *	Text display items.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixDiText.c,v 1.7 2004/03/28 02:44:56 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>

/*
 * If the item has a really small text, or no text at all, use
 * this size. This makes the drawing of selection lines more sane.
 */

#define MIN_TEXT_WIDTH  2
#define MIN_TEXT_HEIGHT 2

/*----------------------------------------------------------------------
 *
 *		Configuration options for Text Items
 *
 *----------------------------------------------------------------------
 */

static Tk_ConfigSpec textItemConfigSpecs[] = {
    {TK_CONFIG_CUSTOM, "-itemtype", "itemType", "ItemType", 
       DEF_TEXTITEM_TYPE, Tk_Offset(TixTextItem, diTypePtr),
       0, &tixConfigItemType},
    {TK_CONFIG_CUSTOM, "-style", "textStyle", "TextStyle",
       DEF_TEXTITEM_STYLE, Tk_Offset(TixTextItem, stylePtr),
       TK_CONFIG_NULL_OK, &tixConfigItemStyle},
    {TK_CONFIG_STRING, "-text", "text", "Text",
       DEF_TEXTITEM_TEXT, Tk_Offset(TixTextItem, text),
       TK_CONFIG_NULL_OK},
    {TK_CONFIG_INT, "-underline", "underline", "Underline",
	DEF_TEXTITEM_UNDERLINE, Tk_Offset(TixTextItem, underline), 0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};


/*----------------------------------------------------------------------
 *
 *		Configuration options for Text Styles
 *
 *----------------------------------------------------------------------
 */

static Tk_ConfigSpec textStyleConfigSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
       DEF_TEXTSTYLE_ANCHOR, Tk_Offset(TixTextStyle, anchor), 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},

#if 0
    /* %bordercolor not used */
    {TK_CONFIG_COLOR,"-bordercolor","borderColor","BorderColor",
       DEF_TEXTSTYLE_BORDER_COLOR_COLOR, Tk_Offset(TixTextStyle, borderColor),
       TK_CONFIG_COLOR_ONLY},

    {TK_CONFIG_COLOR,"-bordercolor","borderColor","BorderColor",
       DEF_TEXTSTYLE_BORDER_COLOR_MONO, Tk_Offset(TixTextStyle, borderColor),
       TK_CONFIG_MONO_ONLY},
#endif

    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},

    {TK_CONFIG_FONT, "-font", "font", "Font",
       DEF_TEXTSTYLE_FONT, Tk_Offset(TixTextStyle, font), 0},

    {TK_CONFIG_JUSTIFY, "-justify", "justify", "Justyfy",
       DEF_TEXTSTYLE_JUSTIFY, Tk_Offset(TixTextStyle, justify),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
       DEF_TEXTSTYLE_PADX, Tk_Offset(TixTextStyle, pad[0]), 0},

    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
       DEF_TEXTSTYLE_PADY, Tk_Offset(TixTextStyle, pad[1]), 0},

    {TK_CONFIG_PIXELS, "-wraplength", "wrapLength", "WrapLength",
       DEF_TEXTSTYLE_WLENGTH, Tk_Offset(TixTextStyle, wrapLength), 0},

/* The following is automatically generated */
	{TK_CONFIG_COLOR,"-background","background","Background",
	DEF_TEXTSTYLE_NORMAL_BG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_NORMAL].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-background","background","Background",
	DEF_TEXTSTYLE_NORMAL_BG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_NORMAL].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-foreground","foreground","Foreground",
	DEF_TEXTSTYLE_NORMAL_FG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_NORMAL].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-foreground","foreground","Foreground",
	DEF_TEXTSTYLE_NORMAL_FG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_NORMAL].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-activebackground","activeBackground","ActiveBackground",
	DEF_TEXTSTYLE_ACTIVE_BG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_ACTIVE].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-activebackground","activeBackground","ActiveBackground",
	DEF_TEXTSTYLE_ACTIVE_BG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_ACTIVE].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-activeforeground","activeForeground","ActiveForeground",
	DEF_TEXTSTYLE_ACTIVE_FG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_ACTIVE].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-activeforeground","activeForeground","ActiveForeground",
	DEF_TEXTSTYLE_ACTIVE_FG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_ACTIVE].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-selectbackground","selectBackground","SelectBackground",
	DEF_TEXTSTYLE_SELECTED_BG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_SELECTED].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-selectbackground","selectBackground","SelectBackground",
	DEF_TEXTSTYLE_SELECTED_BG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_SELECTED].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-selectforeground","selectForeground","SelectForeground",
	DEF_TEXTSTYLE_SELECTED_FG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_SELECTED].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-selectforeground","selectForeground","SelectForeground",
	DEF_TEXTSTYLE_SELECTED_FG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_SELECTED].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-disabledbackground","disabledBackground","DisabledBackground",
	DEF_TEXTSTYLE_DISABLED_BG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_DISABLED].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-disabledbackground","disabledBackground","DisabledBackground",
	DEF_TEXTSTYLE_DISABLED_BG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_DISABLED].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-disabledforeground","disabledForeground","DisabledForeground",
	DEF_TEXTSTYLE_DISABLED_FG_COLOR,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_DISABLED].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-disabledforeground","disabledForeground","DisabledForeground",
	DEF_TEXTSTYLE_DISABLED_FG_MONO,
	Tk_Offset(TixTextStyle,colors[TIX_DITEM_DISABLED].fg),
	TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};


/*----------------------------------------------------------------------
 * Forward declarations for procedures defined later in this file:
 *----------------------------------------------------------------------
 */
static void		Tix_TextItemCalculateSize  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static char *		Tix_TextItemComponent  _ANSI_ARGS_((
			    Tix_DItem * iPtr, int x, int y));
static int		Tix_TextItemConfigure _ANSI_ARGS_((
			    Tix_DItem * iPtr, int argc, CONST84 char ** argv,
			    int flags));
static Tix_DItem *	Tix_TextItemCreate _ANSI_ARGS_((
			    Tix_DispData * ddPtr, Tix_DItemInfo * diTypePtr));
static void		Tix_TextItemDisplay  _ANSI_ARGS_((
			    Drawable drawable, Tix_DItem * iPtr,
			    int x, int y, int width, int height,
                            int xOffset, int yOffset, int flags));
static void		Tix_TextItemFree  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void		Tix_TextItemLostStyle  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void		Tix_TextItemStyleChanged  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static int		Tix_TextStyleConfigure _ANSI_ARGS_((
			    Tix_DItemStyle* style, int argc, CONST84 char ** argv,
			    int flags));
static Tix_DItemStyle *	Tix_TextStyleCreate _ANSI_ARGS_((
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tix_DItemInfo * diTypePtr, char * name));
static void		Tix_TextStyleFree _ANSI_ARGS_((
			    Tix_DItemStyle* style));
static void		Tix_TextStyleSetTemplate _ANSI_ARGS_((
			    Tix_DItemStyle* style,
			    Tix_StyleTemplate * tmplPtr));

Tix_DItemInfo tix_TextItemType = {
    "text",			/* type */
    TIX_DITEM_TEXT,
    Tix_TextItemCreate,		/* createProc */
    Tix_TextItemConfigure,
    Tix_TextItemCalculateSize,
    Tix_TextItemComponent,
    Tix_TextItemDisplay,
    Tix_TextItemFree,
    Tix_TextItemStyleChanged,
    Tix_TextItemLostStyle,

    Tix_TextStyleCreate,
    Tix_TextStyleConfigure,
    Tix_TextStyleFree,
    Tix_TextStyleSetTemplate,

    textItemConfigSpecs,
    textStyleConfigSpecs,
    NULL,				/*next */
};

/*----------------------------------------------------------------------
 * Tix_TextItemCreate --
 *
 *
 *----------------------------------------------------------------------
 */
static Tix_DItem * Tix_TextItemCreate(ddPtr, diTypePtr)
    Tix_DispData * ddPtr;
    Tix_DItemInfo * diTypePtr;
{
    TixTextItem * itPtr;

    itPtr = (TixTextItem*) ckalloc(sizeof(TixTextItem));

    itPtr->diTypePtr	= &tix_TextItemType;
    itPtr->ddPtr	= ddPtr;
    itPtr->stylePtr	= (TixTextStyle*)TixGetDefaultDItemStyle(
	                  itPtr->ddPtr, &tix_TextItemType,
                          (Tix_DItem*)itPtr, NULL);
    itPtr->clientData	= 0;
    itPtr->size[0]	= 0;
    itPtr->size[1]	= 0;
    itPtr->selX         = 0;
    itPtr->selY         = 0;
    itPtr->selW         = 0;
    itPtr->selH         = 0;
    
    itPtr->numChars	= 0;
    itPtr->text		= NULL;
    itPtr->textW	= 0;
    itPtr->textH	= 0;
    itPtr->underline	= -1;

    return (Tix_DItem *)itPtr;
}

static void Tix_TextItemFree(iPtr)
    Tix_DItem * iPtr;
{
    TixTextItem * itPtr = (TixTextItem *) iPtr;

    if (itPtr->stylePtr) {
	TixDItemStyleFree(iPtr, (Tix_DItemStyle*)itPtr->stylePtr);
    }

    Tk_FreeOptions(textItemConfigSpecs, (char *)itPtr,
	itPtr->ddPtr->display, 0);
    ckfree((char*)itPtr);
}

static int
Tix_TextItemConfigure(iPtr, argc, argv, flags)
    Tix_DItem * iPtr;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    TixTextItem * itPtr = (TixTextItem *) iPtr;
    TixTextStyle * oldStyle = itPtr->stylePtr;

    if (Tk_ConfigureWidget(itPtr->ddPtr->interp, itPtr->ddPtr->tkwin,
	textItemConfigSpecs,
	argc, argv, (char *)itPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    if (itPtr->stylePtr == NULL) {
	itPtr->stylePtr = (TixTextStyle*)TixGetDefaultDItemStyle(
	    itPtr->ddPtr, &tix_TextItemType, iPtr, NULL);
    }

    if (oldStyle != NULL && itPtr->stylePtr != oldStyle) {
	Tix_TextItemStyleChanged(iPtr);
    }
    else {
	Tix_TextItemCalculateSize((Tix_DItem*)itPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_TextItemDisplay --
 *
 *	Display a text item. {x, y, width, height} specifies a region
 *      for to display this item in. {xOffset, yOffset} gives the
 *      offset of the top-left corner of the text item relative to
 *      the top-left corder of the region.
 *
 *      Background and foreground of the item are displayed according
 *      to the flags parameter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * 
 *----------------------------------------------------------------------
 */

static void
Tix_TextItemDisplay(drawable, iPtr, x, y, width, height, xOffset, yOffset,
        flags)
    Drawable drawable;
    Tix_DItem * iPtr;
    int x;
    int y;
    int width;
    int height;
    int xOffset;
    int yOffset;
    int flags;
{
    TixTextItem *itPtr = (TixTextItem *)iPtr;
    Display * display = iPtr->base.ddPtr->display;
    TixpSubRegion subReg;
    GC foreGC;

    if ((width <= 0) || (height <= 0)) {
	return;
    }

    TixGetColorDItemGC(iPtr, NULL, &foreGC, NULL, flags);

    TixpStartSubRegionDraw(display, drawable, foreGC,
	    &subReg, 0, 0, x, y, width, height,
	    itPtr->size[0] + xOffset, itPtr->size[1] + yOffset);

    Tix_DItemDrawBackground(drawable, &subReg, iPtr, x, y, width, height,
           xOffset, yOffset, flags);

    /*
     * Calculate the location of the text according to anchor settings.
     */

    TixDItemGetAnchor(iPtr->base.stylePtr->anchor, x, y, width, height,
	    iPtr->base.size[0], iPtr->base.size[1], &x, &y);

    if (foreGC != None && itPtr->text != NULL) {
        /*
         * Draw the text
         */

	x += itPtr->stylePtr->pad[0] + xOffset;
	y += itPtr->stylePtr->pad[1] + yOffset;

	TixpSubRegDisplayText(display, drawable, foreGC,
		&subReg, itPtr->stylePtr->font, itPtr->text,
		itPtr->numChars, x, y, itPtr->textW, itPtr->stylePtr->justify,
		itPtr->underline);
    }

    TixpEndSubRegionDraw(display, drawable, foreGC,
	    &subReg);
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_TextItemComponent --
 *
 *	Identifies the sub-component of this text item at the given
 *      {x, y} location. Text items are not divided into sub-components
 *      so the string "body" is always returned.
 *
 *      The returned string is statically allocated.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 * 
 *----------------------------------------------------------------------
 */

static char *
Tix_TextItemComponent(iPtr, x, y)
    Tix_DItem * iPtr;
    int x;
    int y;
{
    static char * body = "body";

    return body;
}


static void
Tix_TextItemCalculateSize(iPtr)
    Tix_DItem * iPtr;
{
    TixTextItem *itPtr = (TixTextItem *)iPtr;
    char * text = itPtr->text;
    if (text == NULL || *text == '\0') {
        /*
         * Use one space character so that the height of the item
         * would be the same as a regular small item, and the width
         * of the item won't be too tiny.
         */

        text = " ";
    }

    itPtr->numChars = -1;
    TixComputeTextGeometry(itPtr->stylePtr->font, text, -1,
            itPtr->stylePtr->wrapLength, &itPtr->textW, &itPtr->textH);
    itPtr->size[0] = itPtr->textW;
    itPtr->size[1] = itPtr->textH;

    itPtr->size[0] += 2*itPtr->stylePtr->pad[0];
    itPtr->size[1] += 2*itPtr->stylePtr->pad[1];

    itPtr->selX = 0;
    itPtr->selY = 0;
    itPtr->selW = itPtr->size[0];
    itPtr->selH = itPtr->size[1];
}

static void
Tix_TextItemStyleChanged(iPtr)
    Tix_DItem * iPtr;
{
    TixTextItem *itPtr = (TixTextItem *)iPtr;

    if (itPtr->stylePtr == NULL) {
	/* Maybe we haven't set the style to default style yet */
	return;
    }
    Tix_TextItemCalculateSize(iPtr);
    if (itPtr->ddPtr->sizeChangedProc != NULL) {
	itPtr->ddPtr->sizeChangedProc(iPtr);
    }
}

static void
Tix_TextItemLostStyle(iPtr)
    Tix_DItem * iPtr;
{
    TixTextItem *itPtr = (TixTextItem *)iPtr;

    itPtr->stylePtr = (TixTextStyle*)TixGetDefaultDItemStyle(
	itPtr->ddPtr, &tix_TextItemType, iPtr, NULL);

    Tix_TextItemStyleChanged(iPtr);
}
/*----------------------------------------------------------------------
 *
 *
 *			Display styles
 *
 *
 *----------------------------------------------------------------------
 */
static Tix_DItemStyle *
Tix_TextStyleCreate(interp, tkwin, diTypePtr, name)
    Tcl_Interp * interp;
    Tk_Window tkwin;
    char * name;
    Tix_DItemInfo * diTypePtr;
{
    TixTextStyle * stylePtr = (TixTextStyle *)ckalloc(sizeof(TixTextStyle));

    stylePtr->font	 = NULL;
    stylePtr->justify	 = TK_JUSTIFY_LEFT;
    stylePtr->wrapLength = 0;

    return (Tix_DItemStyle *)stylePtr;
}

static int
Tix_TextStyleConfigure(style, argc, argv, flags)
    Tix_DItemStyle *style;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    TixTextStyle * stylePtr = (TixTextStyle *)style;
    XGCValues gcValues;
    GC newGC;
    int i, isNew;

    if (stylePtr->font == NULL) {
	isNew = 1;
    } else {
	isNew = 0;
    }

    /*
     * TODO: gap, wrapLength, etc changes: need to call TixDItemStyleChanged
     */

    if (!(flags &TIX_DONT_CALL_CONFIG)) {
	if (Tk_ConfigureWidget(stylePtr->interp, stylePtr->tkwin,
	    textStyleConfigSpecs,
	    argc, argv, (char *)stylePtr, flags) != TCL_OK) {
	    return TCL_ERROR;
	}
    }

    gcValues.font = TixFontId(stylePtr->font);
    gcValues.graphics_exposures = False;

    for (i=0; i<4; i++) {
	/*
         * Foreground GC
         */

	gcValues.background = stylePtr->colors[i].bg->pixel;
	gcValues.foreground = stylePtr->colors[i].fg->pixel;
	newGC = Tk_GetGC(stylePtr->tkwin,
	    GCFont|GCForeground|GCBackground|GCGraphicsExposures, &gcValues);

	if (stylePtr->colors[i].foreGC != None) {
	    Tk_FreeGC(Tk_Display(stylePtr->tkwin),
		stylePtr->colors[i].foreGC);
	}
	stylePtr->colors[i].foreGC = newGC;

	/*
         * Background GC
         */

	gcValues.foreground = stylePtr->colors[i].bg->pixel;
	newGC = Tk_GetGC(stylePtr->tkwin,
	    GCFont|GCForeground|GCGraphicsExposures, &gcValues);

	if (stylePtr->colors[i].backGC != None) {
	    Tk_FreeGC(Tk_Display(stylePtr->tkwin),
		stylePtr->colors[i].backGC);
	}
	stylePtr->colors[i].backGC = newGC;

        /*
         * Anchor GC
         */

        newGC = Tix_GetAnchorGC(stylePtr->tkwin,
                stylePtr->colors[i].bg);

	if (stylePtr->colors[i].anchorGC != None) {
	    Tk_FreeGC(Tk_Display(stylePtr->tkwin),
		stylePtr->colors[i].anchorGC);
	}
	stylePtr->colors[i].anchorGC = newGC;
    }

    if (!isNew) {
	TixDItemStyleChanged(stylePtr->diTypePtr, (Tix_DItemStyle *)stylePtr);
    }

    return TCL_OK;
}

static void
Tix_TextStyleFree(style)
    Tix_DItemStyle *style;
{
    TixTextStyle * stylePtr = (TixTextStyle *)style;

    Tk_FreeOptions(textStyleConfigSpecs, (char *)stylePtr,
	Tk_Display(stylePtr->tkwin), 0);
    ckfree((char *)stylePtr);
}

static int bg_flags [4] = {
    TIX_DITEM_NORMAL_BG,
    TIX_DITEM_ACTIVE_BG,
    TIX_DITEM_SELECTED_BG,
    TIX_DITEM_DISABLED_BG
};
static int fg_flags [4] = {
    TIX_DITEM_NORMAL_FG,
    TIX_DITEM_ACTIVE_FG,
    TIX_DITEM_SELECTED_FG,
    TIX_DITEM_DISABLED_FG
};


static void
Tix_TextStyleSetTemplate(style, tmplPtr)
    Tix_DItemStyle* style;
    Tix_StyleTemplate * tmplPtr;
{
    TixTextStyle * stylePtr = (TixTextStyle *)style;
    int i;

    if (tmplPtr->flags & TIX_DITEM_FONT) {
	if (stylePtr->font != NULL) {
	    TixFreeFont(stylePtr->font);
	}
	stylePtr->font = TixGetFont(
	    stylePtr->interp, stylePtr->tkwin,
	    TixNameOfFont(tmplPtr->font));
    }
    if (tmplPtr->flags & TIX_DITEM_PADX) {
	stylePtr->pad[0] = tmplPtr->pad[0];
    }
    if (tmplPtr->flags & TIX_DITEM_PADY) {
	stylePtr->pad[1] = tmplPtr->pad[1];
    }

    for (i=0; i<4; i++) {
	if (tmplPtr->flags & bg_flags[i]) {
	    if (stylePtr->colors[i].bg != NULL) {
		Tk_FreeColor(stylePtr->colors[i].bg);
	    }
	    stylePtr->colors[i].bg = Tk_GetColor(
		stylePtr->interp, stylePtr->tkwin,
		Tk_NameOfColor(tmplPtr->colors[i].bg));
	}
    }
    for (i=0; i<4; i++) {
	if (tmplPtr->flags & fg_flags[i]) {
	    if (stylePtr->colors[i].fg != NULL) {
		Tk_FreeColor(stylePtr->colors[i].fg);
	    }
	    stylePtr->colors[i].fg = Tk_GetColor(
		stylePtr->interp, stylePtr->tkwin,
		Tk_NameOfColor(tmplPtr->colors[i].fg));
	}
    }

    Tix_TextStyleConfigure(style, 0, 0, TIX_DONT_CALL_CONFIG);
}
