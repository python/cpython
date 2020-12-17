/*
 * tixDiITxt.c --
 *
 *	This file implements one of the "Display Items" in the Tix library :
 *	Image-text display items.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixDiITxt.c,v 1.6 2004/03/28 02:44:56 hobbs Exp $
 */

#include <tixPort.h>
#include <tk.h>
#include <tixInt.h>
#include <tixDef.h>

#define DEF_IMAGETEXTITEM_BITMAP	""
#define DEF_IMAGETEXTITEM_IMAGE		""
#define DEF_IMAGETEXTITEM_TYPE		"imagetext"
#define DEF_IMAGETEXTITEM_SHOWIMAGE	"1"
#define DEF_IMAGETEXTITEM_SHOWTEXT	"1"
#define DEF_IMAGETEXTITEM_STYLE		""
#define DEF_IMAGETEXTITEM_TEXT		""
#define DEF_IMAGETEXTITEM_UNDERLINE	"-1"

static Tk_ConfigSpec imageTextItemConfigSpecs[] = {

    {TK_CONFIG_BITMAP, "-bitmap", "bitmap", "Bitmap",
       DEF_IMAGETEXTITEM_BITMAP, Tk_Offset(TixImageTextItem, bitmap),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_STRING, "-image", "image", "Image",
       DEF_IMAGETEXTITEM_IMAGE, Tk_Offset(TixImageTextItem, imageString),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_CUSTOM, "-itemtype", "itemType", "ItemType", 
       DEF_IMAGETEXTITEM_TYPE, Tk_Offset(TixImageTextItem, diTypePtr),
       0, &tixConfigItemType},

    {TK_CONFIG_INT, "-showimage", "showImage", "ShowImage",
	DEF_IMAGETEXTITEM_SHOWIMAGE, Tk_Offset(TixImageTextItem, showImage), 0},

    {TK_CONFIG_INT, "-showtext", "showText", "ShowText",
	DEF_IMAGETEXTITEM_SHOWTEXT, Tk_Offset(TixImageTextItem, showText), 0},

    {TK_CONFIG_CUSTOM, "-style", "imageTextStyle", "ImageTextStyle",
       DEF_IMAGETEXTITEM_STYLE, Tk_Offset(TixImageTextItem, stylePtr),
       TK_CONFIG_NULL_OK, &tixConfigItemStyle},

    {TK_CONFIG_STRING, "-text", "text", "Text",
       DEF_IMAGETEXTITEM_TEXT, Tk_Offset(TixImageTextItem, text),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_INT, "-underline", "underline", "Underline",
       DEF_IMAGETEXTITEM_UNDERLINE, Tk_Offset(TixImageTextItem, underline), 0},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*----------------------------------------------------------------------
 *
 *		Configuration options for Text Styles
 *
 *----------------------------------------------------------------------
 */

#define DEF_IMAGETEXTSTYLE_NORMAL_FG_COLOR	NORMAL_FG
#define DEF_IMAGETEXTSTYLE_NORMAL_FG_MONO	BLACK
#define DEF_IMAGETEXTSTYLE_NORMAL_BG_COLOR	TIX_EDITOR_BG
#define DEF_IMAGETEXTSTYLE_NORMAL_BG_MONO	WHITE

#define DEF_IMAGETEXTSTYLE_ACTIVE_FG_COLOR	NORMAL_FG
#define DEF_IMAGETEXTSTYLE_ACTIVE_FG_MONO	WHITE
#define DEF_IMAGETEXTSTYLE_ACTIVE_BG_COLOR	TIX_EDITOR_BG
#define DEF_IMAGETEXTSTYLE_ACTIVE_BG_MONO	BLACK

#define DEF_IMAGETEXTSTYLE_SELECTED_FG_COLOR	SELECT_FG
#define DEF_IMAGETEXTSTYLE_SELECTED_FG_MONO	WHITE
#define DEF_IMAGETEXTSTYLE_SELECTED_BG_COLOR	SELECT_BG
#define DEF_IMAGETEXTSTYLE_SELECTED_BG_MONO	BLACK

#define DEF_IMAGETEXTSTYLE_DISABLED_FG_COLOR	BLACK
#define DEF_IMAGETEXTSTYLE_DISABLED_FG_MONO	BLACK
#define DEF_IMAGETEXTSTYLE_DISABLED_BG_COLOR	TIX_EDITOR_BG
#define DEF_IMAGETEXTSTYLE_DISABLED_BG_MONO	WHITE

#define DEF_IMAGETEXTSTYLE_FONT	        CTL_FONT
#define DEF_IMAGETEXTSTYLE_GAP		"4"
#define DEF_IMAGETEXTSTYLE_PADX		"2"
#define DEF_IMAGETEXTSTYLE_PADY		"2"
#define DEF_IMAGETEXTSTYLE_JUSTIFY	"left"
#define DEF_IMAGETEXTSTYLE_WLENGTH	"0"
#define DEF_IMAGETEXTSTYLE_ANCHOR	"w"


static Tk_ConfigSpec imageTextStyleConfigSpecs[] = {
    {TK_CONFIG_ANCHOR, "-anchor", "anchor", "Anchor",
       DEF_IMAGETEXTSTYLE_ANCHOR, Tk_Offset(TixImageTextStyle, anchor), 0},

    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
       (char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
       (char *) NULL, 0, 0},
 
    {TK_CONFIG_FONT, "-font", "font", "Font",
       DEF_IMAGETEXTSTYLE_FONT, Tk_Offset(TixImageTextStyle, font), 0},

    {TK_CONFIG_PIXELS, "-gap", "gap", "Gap",
       DEF_IMAGETEXTSTYLE_GAP, Tk_Offset(TixImageTextStyle, gap), 0},

    {TK_CONFIG_JUSTIFY, "-justify", "justify", "Justyfy",
       DEF_IMAGETEXTSTYLE_JUSTIFY, Tk_Offset(TixImageTextStyle, justify),
       TK_CONFIG_NULL_OK},

    {TK_CONFIG_PIXELS, "-padx", "padX", "Pad",
       DEF_IMAGETEXTSTYLE_PADX, Tk_Offset(TixImageTextStyle, pad[0]), 0},

    {TK_CONFIG_PIXELS, "-pady", "padY", "Pad",
       DEF_IMAGETEXTSTYLE_PADY, Tk_Offset(TixImageTextStyle, pad[1]), 0},

    {TK_CONFIG_PIXELS, "-wraplength", "wrapLength", "WrapLength",
       DEF_IMAGETEXTSTYLE_WLENGTH, Tk_Offset(TixImageTextStyle, wrapLength),
       0},

/* The following is automatically generated */
	{TK_CONFIG_COLOR,"-background","background","Background",
	DEF_IMAGETEXTSTYLE_NORMAL_BG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_NORMAL].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-background","background","Background",
	DEF_IMAGETEXTSTYLE_NORMAL_BG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_NORMAL].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-foreground","foreground","Foreground",
	DEF_IMAGETEXTSTYLE_NORMAL_FG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_NORMAL].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-foreground","foreground","Foreground",
	DEF_IMAGETEXTSTYLE_NORMAL_FG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_NORMAL].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-activebackground","activeBackground","ActiveBackground",
	DEF_IMAGETEXTSTYLE_ACTIVE_BG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_ACTIVE].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-activebackground","activeBackground","ActiveBackground",
	DEF_IMAGETEXTSTYLE_ACTIVE_BG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_ACTIVE].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-activeforeground","activeForeground","ActiveForeground",
	DEF_IMAGETEXTSTYLE_ACTIVE_FG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_ACTIVE].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-activeforeground","activeForeground","ActiveForeground",
	DEF_IMAGETEXTSTYLE_ACTIVE_FG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_ACTIVE].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-selectbackground","selectBackground","SelectBackground",
	DEF_IMAGETEXTSTYLE_SELECTED_BG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_SELECTED].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-selectbackground","selectBackground","SelectBackground",
	DEF_IMAGETEXTSTYLE_SELECTED_BG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_SELECTED].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-selectforeground","selectForeground","SelectForeground",
	DEF_IMAGETEXTSTYLE_SELECTED_FG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_SELECTED].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-selectforeground","selectForeground","SelectForeground",
	DEF_IMAGETEXTSTYLE_SELECTED_FG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_SELECTED].fg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-disabledbackground","disabledBackground","DisabledBackground",
	DEF_IMAGETEXTSTYLE_DISABLED_BG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_DISABLED].bg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-disabledbackground","disabledBackground","DisabledBackground",
	DEF_IMAGETEXTSTYLE_DISABLED_BG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_DISABLED].bg),
	TK_CONFIG_MONO_ONLY},
	{TK_CONFIG_COLOR,"-disabledforeground","disabledForeground","DisabledForeground",
	DEF_IMAGETEXTSTYLE_DISABLED_FG_COLOR,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_DISABLED].fg),
	TK_CONFIG_COLOR_ONLY},
	{TK_CONFIG_COLOR,"-disabledforeground","disabledForeground","DisabledForeground",
	DEF_IMAGETEXTSTYLE_DISABLED_FG_MONO,
	Tk_Offset(TixImageTextStyle,colors[TIX_DITEM_DISABLED].fg),
	TK_CONFIG_MONO_ONLY},

    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*----------------------------------------------------------------------
 * Forward declarations for procedures defined later in this file:
 *----------------------------------------------------------------------
 */
static void		ImageProc _ANSI_ARGS_((ClientData clientData,
			    int x, int y, int width, int height,
			    int imgWidth, int imgHeight));
static void		Tix_ImageTextItemCalculateSize	_ANSI_ARGS_((
			    Tix_DItem * iPtr));
static char *		Tix_ImageTextItemComponent  _ANSI_ARGS_((
			    Tix_DItem * iPtr, int x, int y));
static int		Tix_ImageTextItemConfigure _ANSI_ARGS_((
			    Tix_DItem * iPtr, int argc, CONST84 char ** argv,
			    int flags));
static Tix_DItem *	Tix_ImageTextItemCreate _ANSI_ARGS_((
			    Tix_DispData * ddPtr, Tix_DItemInfo * diTypePtr));
static void		Tix_ImageTextItemDisplay  _ANSI_ARGS_((
			    Drawable drawable, Tix_DItem * iPtr,
			    int x, int y, int width, int height,
                            int xOffset, int yOffset, int flag));
static void		Tix_ImageTextItemFree  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void		Tix_ImageTextItemLostStyle  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static void		Tix_ImageTextItemStyleChanged  _ANSI_ARGS_((
			    Tix_DItem * iPtr));
static int		Tix_ImageTextStyleConfigure _ANSI_ARGS_((
			    Tix_DItemStyle* style, int argc, CONST84 char ** argv,
			    int flags));
static Tix_DItemStyle *	Tix_ImageTextStyleCreate _ANSI_ARGS_((
			    Tcl_Interp *interp, Tk_Window tkwin,
			    Tix_DItemInfo * diTypePtr, char * name));
static void		Tix_ImageTextStyleFree _ANSI_ARGS_((
			    Tix_DItemStyle* style));
static void		Tix_ImageTextStyleSetTemplate _ANSI_ARGS_((
			    Tix_DItemStyle* style,
			    Tix_StyleTemplate * tmplPtr));

Tix_DItemInfo tix_ImageTextItemType = {
    "imagetext",			/* type */
    TIX_DITEM_IMAGETEXT,
    Tix_ImageTextItemCreate,		/* createProc */
    Tix_ImageTextItemConfigure,
    Tix_ImageTextItemCalculateSize,
    Tix_ImageTextItemComponent,
    Tix_ImageTextItemDisplay,
    Tix_ImageTextItemFree,
    Tix_ImageTextItemStyleChanged,
    Tix_ImageTextItemLostStyle,

    Tix_ImageTextStyleCreate,
    Tix_ImageTextStyleConfigure,
    Tix_ImageTextStyleFree,
    Tix_ImageTextStyleSetTemplate,

    imageTextItemConfigSpecs,
    imageTextStyleConfigSpecs,
    NULL,				/*next */
};


/*----------------------------------------------------------------------
 * Tix_ImageText --
 *
 *
 *----------------------------------------------------------------------
 */
static Tix_DItem * Tix_ImageTextItemCreate(ddPtr, diTypePtr)
    Tix_DispData * ddPtr;
    Tix_DItemInfo * diTypePtr;
{
    TixImageTextItem * itPtr;

    itPtr = (TixImageTextItem*) ckalloc(sizeof(TixImageTextItem));

    itPtr->diTypePtr	= diTypePtr;
    itPtr->ddPtr	= ddPtr;
    itPtr->stylePtr	= NULL;
    itPtr->clientData	= 0;
    itPtr->size[0]	= 0;
    itPtr->size[1]	= 0;

    itPtr->bitmap	= None;
    itPtr->bitmapW	= 0;
    itPtr->bitmapH	= 0;

    itPtr->imageString	= NULL;
    itPtr->image	= NULL;
    itPtr->imageW	= 0;
    itPtr->imageH	= 0;

    itPtr->numChars	= 0;            /* TODO: this is currently not used */
    itPtr->text		= NULL;
    itPtr->textW	= 0;
    itPtr->textH	= 0;
    itPtr->underline	= -1;

    itPtr->showImage	= 1;
    itPtr->showText	= 1;

    return (Tix_DItem *)itPtr;
}

static void Tix_ImageTextItemFree(iPtr)
    Tix_DItem * iPtr;
{
    TixImageTextItem * itPtr = (TixImageTextItem *) iPtr;

    if (itPtr->image) {
	Tk_FreeImage(itPtr->image);
    }
    if (itPtr->stylePtr) {
	TixDItemStyleFree(iPtr, (Tix_DItemStyle*)itPtr->stylePtr);
    }

    Tk_FreeOptions(imageTextItemConfigSpecs, (char *)itPtr,
	itPtr->ddPtr->display, 0);
    ckfree((char*)itPtr);
}

static int Tix_ImageTextItemConfigure(iPtr, argc, argv, flags)
    Tix_DItem * iPtr;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    TixImageTextItem * itPtr = (TixImageTextItem *) iPtr;
    TixImageTextStyle * oldStyle = itPtr->stylePtr;

    if (Tk_ConfigureWidget(itPtr->ddPtr->interp, itPtr->ddPtr->tkwin,
	imageTextItemConfigSpecs,
	argc, argv, (char *)itPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }
    if (itPtr->stylePtr == NULL) {
	itPtr->stylePtr = (TixImageTextStyle*)TixGetDefaultDItemStyle(
	    itPtr->ddPtr, &tix_ImageTextItemType, iPtr, NULL);
    }

    /*
     * Free the old images for the widget, if there were any.
     */
    if (itPtr->image != NULL) {
	Tk_FreeImage(itPtr->image);
	itPtr->image = NULL;
    }

    if (itPtr->imageString != NULL) {
	itPtr->image = Tk_GetImage(itPtr->ddPtr->interp, itPtr->ddPtr->tkwin,
	    itPtr->imageString, ImageProc, (ClientData) itPtr);
	if (itPtr->image == NULL) {
	    return TCL_ERROR;
	}
    }

    if (oldStyle != NULL && itPtr->stylePtr != oldStyle) {
	Tix_ImageTextItemStyleChanged(iPtr);
    }
    else {
	Tix_ImageTextItemCalculateSize((Tix_DItem*)itPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tix_ImageTextItemDisplay --
 *
 *      Display an imagetext item. {x, y, width, height} specifies a
 *      region for to display this item in. {xOffset, yOffset} gives
 *      the offset of the top-left corner of the text item relative
 *      to the top-left corder of the region.
 *
 *      Background and foreground of the item are displayed according
 *      to the flags parameter.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 * 
 *----------------------------------------------------------------------
 */

static void
Tix_ImageTextItemDisplay(drawable, iPtr, x, y, width, height,
        xOffset, yOffset, flags)
    Drawable drawable;          /* Where to display this item */
    Tix_DItem * iPtr;           /* Item to display */
    int x;                      /* x pos of top-left corner of region
                                 * to display item in */
    int y;                      /* y pos of top-left corner of region */
    int width;                  /* width of region */
    int height;                 /* height of region */
    int xOffset;                /* X offset of item within region */
    int yOffset;                /* Y offset of item within region */
    int flags;                  /* Controls how fg/bg/anchor lines are
                                 * drawn */
{
    TixImageTextItem *itPtr = (TixImageTextItem *)iPtr;
    GC foreGC, bitmapGC;
    TixpSubRegion subReg;
    Display * display = itPtr->ddPtr->display;

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
     * Calculate the location of the item body according to anchor settings.
     */

    TixDItemGetAnchor(iPtr->base.stylePtr->anchor, x, y, width, height,
	    iPtr->base.size[0], iPtr->base.size[1], &x, &y);

    x += xOffset;
    y += yOffset;

    /*
     * Draw the foreground items
     */

    if (itPtr->image != NULL) {
	int bitY;

	bitY = itPtr->size[1] - itPtr->imageH - 2*itPtr->stylePtr->pad[1];

	if (bitY > 0) {
	    bitY = bitY / 2 + (bitY %2);
	} else {
	    bitY = 0;
	}
	if (itPtr->showImage && foreGC != None) {
	    TixpSubRegDrawImage(&subReg, itPtr->image, 0, 0,
		    itPtr->imageW, itPtr->imageH, drawable,
		    x + itPtr->stylePtr->pad[0],
		    y + itPtr->stylePtr->pad[1] + bitY);
	}
	x += itPtr->imageW + itPtr->stylePtr->gap;
    }
    else if (itPtr->bitmap != None) {
	int bitY;

	bitY = itPtr->size[1] - itPtr->bitmapH - 2*itPtr->stylePtr->pad[1];
	if (bitY > 0) {
	    bitY = bitY / 2 + (bitY %2);
	} else {
	    bitY = 0;
	}

	if (itPtr->showImage && foreGC != None) {
            if ((flags & TIX_DITEM_ALL_BG) != 0) {
                /*
                 * If we draw the background, the bitmap is never
                 * displayed as selected, so we choose the normal GC.
                 */

                bitmapGC = itPtr->stylePtr->colors[TIX_DITEM_NORMAL].foreGC;
            } else {
                /*
                 * The caller has already drawn the background. foreGC
                 * is the most compatible GC to be used with the background.
                 */

                bitmapGC = foreGC;
            }

	    TixpSubRegDrawBitmap(display, drawable, bitmapGC,
		    &subReg, itPtr->bitmap, 0, 0,
		    itPtr->bitmapW, itPtr->bitmapH,
		    x + itPtr->stylePtr->pad[0],
		    y + itPtr->stylePtr->pad[1] + bitY,
		    1);
	}
	x += itPtr->bitmapW + itPtr->stylePtr->gap;
    }

    if (itPtr->text && itPtr->showText && foreGC != None) {
	int textY;
	
	textY = itPtr->size[1] - itPtr->textH - 2*itPtr->stylePtr->pad[1];
	if (textY > 0) {
	    textY = textY / 2 + (textY %2);
	} else {
	    textY = 0;
	}

	TixpSubRegDisplayText(display, drawable, foreGC, &subReg,
		itPtr->stylePtr->font, itPtr->text, -1,
		x + itPtr->stylePtr->pad[0],
		y + itPtr->stylePtr->pad[1] + textY,
		itPtr->textW,
		itPtr->stylePtr->justify,
		itPtr->underline);
    }

    TixpEndSubRegionDraw(display, drawable, foreGC,
	    &subReg);
}

static void
Tix_ImageTextItemCalculateSize(iPtr)
    Tix_DItem * iPtr;
{
    TixImageTextItem *itPtr = (TixImageTextItem *)iPtr;
    char * text;

    itPtr->size[0] = 0;
    itPtr->size[1] = 0;

    /*
     * Note: the size of the image or the text are used even when
     * the showImage or showText options are off. These two options are 
     * used to "blank" the respective components temporarily without
     * affecting the geometry of the ditem. The main is to indicate
     * transfer during drag+drop.
     *
     * If you want the image or text to completely disappear, config them
     * to NULL
     */

    if (itPtr->image != NULL) {
	Tk_SizeOfImage(itPtr->image, &itPtr->imageW, &itPtr->imageH);

	itPtr->size[0] = itPtr->imageW + itPtr->stylePtr->gap;
	itPtr->size[1] = itPtr->imageH;
    }
    else if (itPtr->bitmap != None) {
	Tk_SizeOfBitmap(itPtr->ddPtr->display, itPtr->bitmap, &itPtr->bitmapW,
		&itPtr->bitmapH);

	itPtr->size[0] = itPtr->bitmapW + itPtr->stylePtr->gap;
	itPtr->size[1] = itPtr->bitmapH;
    }

    text = itPtr->text;
    if (text == NULL || text[0] == '\0') {
        /*
         * Use one space character so that the height of the item
         * would be the same as a regular small item, and the width
         * of the item won't be too tiny.
         */

        text = " ";
    }

    TixComputeTextGeometry(itPtr->stylePtr->font, text,
            -1, itPtr->stylePtr->wrapLength,
            &itPtr->textW, &itPtr->textH);

    itPtr->size[0] += itPtr->textW;
	
    if (itPtr->textH > itPtr->size[1]) {
        itPtr->size[1] = itPtr->textH;
    }

    itPtr->size[0] += 2*itPtr->stylePtr->pad[0];
    itPtr->size[1] += 2*itPtr->stylePtr->pad[1];

    itPtr->selX = 0;
    itPtr->selY = 0;
    itPtr->selW = itPtr->size[0];
    itPtr->selH = itPtr->size[1];

    if (itPtr->image != NULL) {
        itPtr->selX = itPtr->imageW + itPtr->stylePtr->gap;
        itPtr->selW -= itPtr->selX;
    } else if (itPtr->bitmap != None) {
        itPtr->selX = itPtr->bitmapW + itPtr->stylePtr->gap;
        itPtr->selW -= itPtr->selX;
    }
}

static char * Tix_ImageTextItemComponent(iPtr, x, y)
    Tix_DItem * iPtr;
    int x;
    int y;
{
    /* TODO: Unimplemented */
#if 0
    TixImageTextItem *itPtr = (TixImageTextItem *)iPtr;
#endif

    static char * body = "body";

    return body;
}


static void Tix_ImageTextItemStyleChanged(iPtr)
    Tix_DItem * iPtr;
{
    TixImageTextItem *itPtr = (TixImageTextItem *)iPtr;

    if (itPtr->stylePtr == NULL) {
	/* Maybe we haven't set the style to default style yet */
	return;
    }
    Tix_ImageTextItemCalculateSize(iPtr);
    if (itPtr->ddPtr->sizeChangedProc != NULL) {
	itPtr->ddPtr->sizeChangedProc(iPtr);
    }
}
static void Tix_ImageTextItemLostStyle(iPtr)
    Tix_DItem * iPtr;
{
    TixImageTextItem *itPtr = (TixImageTextItem *)iPtr;

    itPtr->stylePtr = (TixImageTextStyle*)TixGetDefaultDItemStyle(
	itPtr->ddPtr, &tix_ImageTextItemType, iPtr, NULL);

    Tix_ImageTextItemStyleChanged(iPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ImageProc --
 *
 *	This procedure is invoked by the image code whenever the manager
 *	for an image does something that affects the size of contents
 *	of an image displayed in this widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for the HList to get redisplayed.
 *
 *----------------------------------------------------------------------
 */
static void
ImageProc(clientData, x, y, width, height, imgWidth, imgHeight)
    ClientData clientData;		/* Pointer to widget record. */
    int x, y;				/* Upper left pixel (within image)
					 * that must be redisplayed. */
    int width, height;			/* Dimensions of area to redisplay
					 * (may be <= 0). */
    int imgWidth, imgHeight;		/* New dimensions of image. */
{
    TixImageTextItem *itPtr = (TixImageTextItem *)clientData;

    Tix_ImageTextItemCalculateSize((Tix_DItem *)itPtr);
    if (itPtr->ddPtr->sizeChangedProc != NULL) {
	itPtr->ddPtr->sizeChangedProc((Tix_DItem *)itPtr);
    }
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
Tix_ImageTextStyleCreate(interp, tkwin, diTypePtr, name)
    Tcl_Interp * interp;
    Tk_Window tkwin;
    char * name;
    Tix_DItemInfo * diTypePtr;
{
    TixImageTextStyle * stylePtr =
      (TixImageTextStyle *)ckalloc(sizeof(TixImageTextStyle));

    stylePtr->font	 = NULL;
    stylePtr->gap	 = 0;
    stylePtr->justify	 = TK_JUSTIFY_LEFT;
    stylePtr->wrapLength = 0;

    return (Tix_DItemStyle *)stylePtr;
}

static int
Tix_ImageTextStyleConfigure(style, argc, argv, flags)
    Tix_DItemStyle *style;
    int argc;
    CONST84 char ** argv;
    int flags;
{
    TixImageTextStyle * stylePtr = (TixImageTextStyle *)style;
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
	    imageTextStyleConfigSpecs,
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

static void Tix_ImageTextStyleFree(style)
    Tix_DItemStyle *style;
{
    TixImageTextStyle * stylePtr = (TixImageTextStyle *)style;

    Tk_FreeOptions(imageTextStyleConfigSpecs, (char *)stylePtr,
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
Tix_ImageTextStyleSetTemplate(style, tmplPtr)
    Tix_DItemStyle* style;
    Tix_StyleTemplate * tmplPtr;
{
    TixImageTextStyle * stylePtr = (TixImageTextStyle *)style;
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

    Tix_ImageTextStyleConfigure(style, 0, 0, TIX_DONT_CALL_CONFIG);
}
