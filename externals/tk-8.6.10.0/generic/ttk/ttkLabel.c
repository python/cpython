/*
 * text, image, and label elements.
 *
 * The label element combines text and image elements,
 * with layout determined by the "-compound" option.
 *
 */

#include "tkInt.h"
#include "ttkTheme.h"

/*----------------------------------------------------------------------
 * +++ Text element.
 *
 * This element displays a textual label in the foreground color.
 *
 * Optionally underlines the mnemonic character if the -underline resource
 * is present and >= 0.
 */

typedef struct {
    /*
     * Element options:
     */
    Tcl_Obj	*textObj;
    Tcl_Obj	*fontObj;
    Tcl_Obj	*foregroundObj;
    Tcl_Obj	*underlineObj;
    Tcl_Obj	*widthObj;
    Tcl_Obj	*anchorObj;
    Tcl_Obj	*justifyObj;
    Tcl_Obj	*wrapLengthObj;
    Tcl_Obj     *embossedObj;

    /*
     * Computed resources:
     */
    Tk_Font		tkfont;
    Tk_TextLayout	textLayout;
    int 		width;
    int 		height;
    int			embossed;

} TextElement;

/* Text element options table.
 * NB: Keep in sync with label element option table.
 */
static Ttk_ElementOptionSpec TextElementOptions[] = {
    { "-text", TK_OPTION_STRING,
	Tk_Offset(TextElement,textObj), "" },
    { "-font", TK_OPTION_FONT,
	Tk_Offset(TextElement,fontObj), DEFAULT_FONT },
    { "-foreground", TK_OPTION_COLOR,
	Tk_Offset(TextElement,foregroundObj), "black" },
    { "-underline", TK_OPTION_INT,
	Tk_Offset(TextElement,underlineObj), "-1"},
    { "-width", TK_OPTION_INT,
	Tk_Offset(TextElement,widthObj), "-1"},
    { "-anchor", TK_OPTION_ANCHOR,
	Tk_Offset(TextElement,anchorObj), "w"},
    { "-justify", TK_OPTION_JUSTIFY,
	Tk_Offset(TextElement,justifyObj), "left" },
    { "-wraplength", TK_OPTION_PIXELS,
	Tk_Offset(TextElement,wrapLengthObj), "0" },
    { "-embossed", TK_OPTION_INT,
	Tk_Offset(TextElement,embossedObj), "0"},
    { NULL, 0, 0, NULL }
};

static int TextSetup(TextElement *text, Tk_Window tkwin)
{
    const char *string = Tcl_GetString(text->textObj);
    Tk_Justify justify = TK_JUSTIFY_LEFT;
    int wrapLength = 0;

    text->tkfont = Tk_GetFontFromObj(tkwin, text->fontObj);
    Tk_GetJustifyFromObj(NULL, text->justifyObj, &justify);
    Tk_GetPixelsFromObj(NULL, tkwin, text->wrapLengthObj, &wrapLength);
    Tcl_GetBooleanFromObj(NULL, text->embossedObj, &text->embossed);

    text->textLayout = Tk_ComputeTextLayout(
	    text->tkfont, string, -1/*numChars*/, wrapLength, justify,
	    0/*flags*/, &text->width, &text->height);

    return 1;
}

/*
 * TextReqWidth -- compute the requested width of a text element.
 *
 * If -width is positive, use that as the width
 * If -width is negative, use that as the minimum width
 * If not specified or empty, use the natural size of the text
 */

static int TextReqWidth(TextElement *text)
{
    int reqWidth;

    if (   text->widthObj
	&& Tcl_GetIntFromObj(NULL, text->widthObj, &reqWidth) == TCL_OK)
    {
	int avgWidth = Tk_TextWidth(text->tkfont, "0", 1);
	if (reqWidth <= 0) {
	    int specWidth = avgWidth * -reqWidth;
	    if (specWidth > text->width)
		return specWidth;
	} else {
	    return avgWidth * reqWidth;
	}
    }
    return text->width;
}

static void TextCleanup(TextElement *text)
{
    Tk_FreeTextLayout(text->textLayout);
}

/*
 * TextDraw --
 * 	Draw a text element.
 * 	Called by TextElementDraw() and LabelElementDraw().
 */
static void TextDraw(TextElement *text, Tk_Window tkwin, Drawable d, Ttk_Box b)
{
    XColor *color = Tk_GetColorFromObj(tkwin, text->foregroundObj);
    int underline = -1;
    XGCValues gcValues;
    GC gc1, gc2;
    Tk_Anchor anchor = TK_ANCHOR_CENTER;
    TkRegion clipRegion = NULL;

    gcValues.font = Tk_FontId(text->tkfont);
    gcValues.foreground = color->pixel;
    gc1 = Tk_GetGC(tkwin, GCFont | GCForeground, &gcValues);
    gcValues.foreground = WhitePixelOfScreen(Tk_Screen(tkwin));
    gc2 = Tk_GetGC(tkwin, GCFont | GCForeground, &gcValues);

    /* 
     * Place text according to -anchor:
     */
    Tk_GetAnchorFromObj(NULL, text->anchorObj, &anchor);
    b = Ttk_AnchorBox(b, text->width, text->height, anchor);

    /*
     * Clip text if it's too wide:
     */
    if (b.width < text->width) {
	XRectangle rect;

	clipRegion = TkCreateRegion();
	rect.x = b.x;
	rect.y = b.y;
	rect.width = b.width + (text->embossed ? 1 : 0);
	rect.height = b.height + (text->embossed ? 1 : 0);
	TkUnionRectWithRegion(&rect, clipRegion, clipRegion);
	TkSetRegion(Tk_Display(tkwin), gc1, clipRegion);
	TkSetRegion(Tk_Display(tkwin), gc2, clipRegion);
#ifdef HAVE_XFT
	TkUnixSetXftClipRegion(clipRegion);
#endif
    }

    if (text->embossed) {
	Tk_DrawTextLayout(Tk_Display(tkwin), d, gc2,
	    text->textLayout, b.x+1, b.y+1, 0/*firstChar*/, -1/*lastChar*/);
    }
    Tk_DrawTextLayout(Tk_Display(tkwin), d, gc1,
	    text->textLayout, b.x, b.y, 0/*firstChar*/, -1/*lastChar*/);

    Tcl_GetIntFromObj(NULL, text->underlineObj, &underline);
    if (underline >= 0) {
	if (text->embossed) {
	    Tk_UnderlineTextLayout(Tk_Display(tkwin), d, gc2,
		text->textLayout, b.x+1, b.y+1, underline);
	}
	Tk_UnderlineTextLayout(Tk_Display(tkwin), d, gc1,
	    text->textLayout, b.x, b.y, underline);
    }

    if (clipRegion != NULL) {
#ifdef HAVE_XFT
	TkUnixSetXftClipRegion(NULL);
#endif
	XSetClipMask(Tk_Display(tkwin), gc1, None);
	XSetClipMask(Tk_Display(tkwin), gc2, None);
	TkDestroyRegion(clipRegion);
    }
    Tk_FreeGC(Tk_Display(tkwin), gc1);
    Tk_FreeGC(Tk_Display(tkwin), gc2);
}

static void TextElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    TextElement *text = elementRecord;

    if (!TextSetup(text, tkwin))
	return;

    *heightPtr = text->height;
    *widthPtr = TextReqWidth(text);

    TextCleanup(text);

    return;
}

static void TextElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    TextElement *text = elementRecord;
    if (TextSetup(text, tkwin)) {
	TextDraw(text, tkwin, d, b);
	TextCleanup(text);
    }
}

static Ttk_ElementSpec TextElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(TextElement),
    TextElementOptions,
    TextElementSize,
    TextElementDraw
};

/*----------------------------------------------------------------------
 * +++ Image element.
 * Draws an image.
 */

typedef struct {
    Tcl_Obj	*imageObj;
    Tcl_Obj 	*stippleObj;	/* For TTK_STATE_DISABLED */
    Tcl_Obj 	*backgroundObj;	/* " " */

    Ttk_ImageSpec *imageSpec;
    Tk_Image	tkimg;
    int 	width;
    int		height;
} ImageElement;

/* ===> NB: Keep in sync with label element option table.  <===
 */
static Ttk_ElementOptionSpec ImageElementOptions[] = {
    { "-image", TK_OPTION_STRING,
	Tk_Offset(ImageElement,imageObj), "" },
    { "-stipple", TK_OPTION_STRING, 	/* Really: TK_OPTION_BITMAP */
	Tk_Offset(ImageElement,stippleObj), "gray50" },
    { "-background", TK_OPTION_COLOR,
	Tk_Offset(ImageElement,backgroundObj), DEFAULT_BACKGROUND },
    { NULL, 0, 0, NULL }
};

/*
 * ImageSetup() --
 * 	Look up the Tk_Image from the image element's imageObj resource.
 * 	Caller must release the image with ImageCleanup().
 *
 * Returns:
 * 	1 if successful, 0 if there was an error (unreported)
 * 	or the image resource was not specified.
 */

static int ImageSetup(
    ImageElement *image, Tk_Window tkwin, Ttk_State state)
{

    if (!image->imageObj) {
	return 0;
    }
    image->imageSpec = TtkGetImageSpec(NULL, tkwin, image->imageObj);
    if (!image->imageSpec) {
	return 0;
    }
    image->tkimg = TtkSelectImage(image->imageSpec, state);
    if (!image->tkimg) {
	TtkFreeImageSpec(image->imageSpec);
	return 0;
    }
    Tk_SizeOfImage(image->tkimg, &image->width, &image->height);

    return 1;
}

static void ImageCleanup(ImageElement *image)
{
    TtkFreeImageSpec(image->imageSpec);
}

#ifndef MAC_OSX_TK
/*
 * StippleOver --
 * 	Draw a stipple over the image area, to make it look "grayed-out"
 * 	when TTK_STATE_DISABLED is set.
 */
static void StippleOver(
    ImageElement *image, Tk_Window tkwin, Drawable d, int x, int y)
{
    Pixmap stipple = Tk_AllocBitmapFromObj(NULL, tkwin, image->stippleObj);
    XColor *color = Tk_GetColorFromObj(tkwin, image->backgroundObj);

    if (stipple != None) {
	unsigned long mask = GCFillStyle | GCStipple | GCForeground;
	XGCValues gcvalues;
	GC gc;
	gcvalues.foreground = color->pixel;
	gcvalues.fill_style = FillStippled;
	gcvalues.stipple = stipple;
	gc = Tk_GetGC(tkwin, mask, &gcvalues);
	XFillRectangle(Tk_Display(tkwin),d,gc,x,y,image->width,image->height);
	Tk_FreeGC(Tk_Display(tkwin), gc);
	Tk_FreeBitmapFromObj(tkwin, image->stippleObj);
    }
}
#endif

static void ImageDraw(
    ImageElement *image, Tk_Window tkwin,Drawable d,Ttk_Box b,Ttk_State state)
{
    int width = image->width, height = image->height;

    /* Clip width and height to remain within window bounds:
     */
    if (b.x + width > Tk_Width(tkwin)) {
	width = Tk_Width(tkwin) - b.x;
    }
    if (b.y + height > Tk_Height(tkwin)) {
	height = Tk_Height(tkwin) - b.y;
    }

    if (height <= 0 || width <= 0) {
	/* Completely clipped - bail out.
	 */
	return;
    }

    Tk_RedrawImage(image->tkimg, 0,0, width, height, d, b.x, b.y);

    /* If we're disabled there's no state-specific 'disabled' image, 
     * stipple the image.
     * @@@ Possibly: Don't do disabled-stippling at all;
     * @@@ it's ugly and out of fashion.
     * Do not stipple at all under Aqua, just draw the image: it shows up 
     * as a white rectangle otherwise.
     */

    
    if (state & TTK_STATE_DISABLED) {
	if (TtkSelectImage(image->imageSpec, 0ul) == image->tkimg) {
#ifndef MAC_OSX_TK
	    StippleOver(image, tkwin, d, b.x,b.y);
#endif
	}
    }
}

static void ImageElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ImageElement *image = elementRecord;

    if (ImageSetup(image, tkwin, 0)) {
	*widthPtr = image->width;
	*heightPtr = image->height;
	ImageCleanup(image);
    }
}

static void ImageElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    ImageElement *image = elementRecord;

    if (ImageSetup(image, tkwin, state)) {
	ImageDraw(image, tkwin, d, b, state);
	ImageCleanup(image);
    }
}

static Ttk_ElementSpec ImageElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(ImageElement),
    ImageElementOptions,
    ImageElementSize,
    ImageElementDraw
};

/*------------------------------------------------------------------------
 * +++ Label element.
 *
 * Displays an image and/or text, as determined by the -compound option.
 *
 * Differences from Tk 8.4 compound elements:
 *
 * This adds two new values for the -compound option, "text"
 * and "image".  (This is useful for configuring toolbars to
 * display icons, text and icons, or text only, as found in
 * many browsers.)
 *
 * "-compound none" is supported, but I'd like to get rid of it;
 * it makes the logic more complex, and the only benefit is
 * backwards compatibility with Tk < 8.3.0 scripts.
 *
 * This adds a new resource, -space, for determining how much
 * space to leave between the text and image; Tk 8.4 reuses the
 * -padx or -pady option for this purpose.
 *
 * -width always specifies the length in characters of the text part;
 *  in Tk 8.4 it's either characters or pixels, depending on the
 *  value of -compound.
 *
 * Negative values of -width are interpreted as a minimum width
 * on all platforms, not just on Windows.
 *
 * Tk 8.4 ignores -padx and -pady if -compound is set to "none".
 * Here, padding is handled by a different element.
 */

typedef struct {
    /*
     * Element options:
     */
    Tcl_Obj		*compoundObj;
    Tcl_Obj		*spaceObj;
    TextElement 	text;
    ImageElement	image;

    /*
     * Computed values (see LabelSetup)
     */
    Ttk_Compound	compound;
    int  		space;
    int 		totalWidth, totalHeight;
} LabelElement;

static Ttk_ElementOptionSpec LabelElementOptions[] = {
    { "-compound", TK_OPTION_ANY,
	Tk_Offset(LabelElement,compoundObj), "none" },
    { "-space", TK_OPTION_PIXELS,
	Tk_Offset(LabelElement,spaceObj), "4" },

    /* Text element part:
     * NB: Keep in sync with TextElementOptions.
     */
    { "-text", TK_OPTION_STRING,
	Tk_Offset(LabelElement,text.textObj), "" },
    { "-font", TK_OPTION_FONT,
	Tk_Offset(LabelElement,text.fontObj), DEFAULT_FONT },
    { "-foreground", TK_OPTION_COLOR,
	Tk_Offset(LabelElement,text.foregroundObj), "black" },
    { "-underline", TK_OPTION_INT,
	Tk_Offset(LabelElement,text.underlineObj), "-1"},
    { "-width", TK_OPTION_INT,
	Tk_Offset(LabelElement,text.widthObj), ""},
    { "-anchor", TK_OPTION_ANCHOR,
	Tk_Offset(LabelElement,text.anchorObj), "w"},
    { "-justify", TK_OPTION_JUSTIFY,
	Tk_Offset(LabelElement,text.justifyObj), "left" },
    { "-wraplength", TK_OPTION_PIXELS,
	Tk_Offset(LabelElement,text.wrapLengthObj), "0" },
    { "-embossed", TK_OPTION_INT,
	Tk_Offset(LabelElement,text.embossedObj), "0"},

    /* Image element part:
     * NB: Keep in sync with ImageElementOptions.
     */
    { "-image", TK_OPTION_STRING,
	Tk_Offset(LabelElement,image.imageObj), "" },
    { "-stipple", TK_OPTION_STRING, 	/* Really: TK_OPTION_BITMAP */
	Tk_Offset(LabelElement,image.stippleObj), "gray50" },
    { "-background", TK_OPTION_COLOR,
	Tk_Offset(LabelElement,image.backgroundObj), DEFAULT_BACKGROUND },
    { NULL, 0, 0, NULL }
};

/*
 * LabelSetup --
 * 	Fills in computed fields of the label element.
 *
 * 	Calculate the text, image, and total width and height.
 */

#undef  MAX
#define MAX(a,b) ((a) > (b) ? a : b);
static void LabelSetup(
    LabelElement *c, Tk_Window tkwin, Ttk_State state)
{
    Ttk_Compound *compoundPtr = &c->compound;

    Tk_GetPixelsFromObj(NULL,tkwin,c->spaceObj,&c->space);
    Ttk_GetCompoundFromObj(NULL,c->compoundObj,(int*)compoundPtr);

    /*
     * Deal with TTK_COMPOUND_NONE.
     */
    if (c->compound == TTK_COMPOUND_NONE) {
	if (ImageSetup(&c->image, tkwin, state)) {
	    c->compound = TTK_COMPOUND_IMAGE;
	} else {
	    c->compound = TTK_COMPOUND_TEXT;
	}
    } else if (c->compound != TTK_COMPOUND_TEXT) {
    	if (!ImageSetup(&c->image, tkwin, state)) {
	    c->compound = TTK_COMPOUND_TEXT;
	}
    }
    if (c->compound != TTK_COMPOUND_IMAGE)
	TextSetup(&c->text, tkwin);

    /*
     * ASSERT:
     * if c->compound != IMAGE, then TextSetup() has been called
     * if c->compound != TEXT, then ImageSetup() has returned successfully
     * c->compound != COMPOUND_NONE.
     */

    switch (c->compound)
    {
	case TTK_COMPOUND_NONE:
	    /* Can't happen */
	    break;
	case TTK_COMPOUND_TEXT:
	    c->totalWidth  = c->text.width;
	    c->totalHeight = c->text.height;
	    break;
	case TTK_COMPOUND_IMAGE:
	    c->totalWidth  = c->image.width;
	    c->totalHeight = c->image.height;
	    break;
	case TTK_COMPOUND_CENTER:
	    c->totalWidth  = MAX(c->image.width, c->text.width);
	    c->totalHeight = MAX(c->image.height, c->text.height);
	    break;
	case TTK_COMPOUND_TOP:
	case TTK_COMPOUND_BOTTOM:
	    c->totalWidth  = MAX(c->image.width, c->text.width);
	    c->totalHeight = c->image.height + c->text.height + c->space;
	    break;

	case TTK_COMPOUND_LEFT:
	case TTK_COMPOUND_RIGHT:
	    c->totalWidth  = c->image.width + c->text.width + c->space;
	    c->totalHeight = MAX(c->image.height, c->text.height);
	    break;
    }
}

static void LabelCleanup(LabelElement *c)
{
    if (c->compound != TTK_COMPOUND_TEXT)
	ImageCleanup(&c->image);
    if (c->compound != TTK_COMPOUND_IMAGE)
	TextCleanup(&c->text);
}

static void LabelElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    LabelElement *label = elementRecord;
    int textReqWidth = 0;

    LabelSetup(label, tkwin, 0);

    *heightPtr = label->totalHeight;

    /* Requested width based on -width option, not actual text width:
     */
    if (label->compound != TTK_COMPOUND_IMAGE)
	textReqWidth = TextReqWidth(&label->text);

    switch (label->compound) 
    {
	case TTK_COMPOUND_TEXT:
	    *widthPtr = textReqWidth;
	    break;
	case TTK_COMPOUND_IMAGE:
	    *widthPtr = label->image.width;
	    break;
	case TTK_COMPOUND_TOP:
	case TTK_COMPOUND_BOTTOM:
	case TTK_COMPOUND_CENTER:
	    *widthPtr = MAX(label->image.width, textReqWidth); 
	    break;
	case TTK_COMPOUND_LEFT:
	case TTK_COMPOUND_RIGHT:
	    *widthPtr = label->image.width + textReqWidth + label->space; 
	    break;
	case TTK_COMPOUND_NONE:
	    break; /* Can't happen */
    }

    LabelCleanup(label);
}

/*
 * DrawCompound --
 * 	Helper routine for LabelElementDraw;
 * 	Handles layout for -compound {left,right,top,bottom}
 */
static void DrawCompound(
    LabelElement *l, Ttk_Box b, Tk_Window tkwin, Drawable d, Ttk_State state,
    int imageSide, int textSide)
{
    Ttk_Box imageBox =
	Ttk_PlaceBox(&b, l->image.width, l->image.height, imageSide, 0);
    Ttk_Box textBox =
	Ttk_PlaceBox(&b, l->text.width, l->text.height, textSide, 0);
    ImageDraw(&l->image,tkwin,d,imageBox,state);
    TextDraw(&l->text,tkwin,d,textBox);
}

static void LabelElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    LabelElement *l = elementRecord;
    Tk_Anchor anchor = TK_ANCHOR_CENTER;

    LabelSetup(l, tkwin, state);

    /*
     * Adjust overall parcel based on -anchor:
     */
    Tk_GetAnchorFromObj(NULL, l->text.anchorObj, &anchor);
    b = Ttk_AnchorBox(b, l->totalWidth, l->totalHeight, anchor);

    /*
     * Draw text and/or image parts based on -compound:
     */
    switch (l->compound)
    {
	case TTK_COMPOUND_NONE:
	    /* Can't happen */
	    break;
	case TTK_COMPOUND_TEXT:
	    TextDraw(&l->text,tkwin,d,b);
	    break;
	case TTK_COMPOUND_IMAGE:
	    ImageDraw(&l->image,tkwin,d,b,state);
	    break;
	case TTK_COMPOUND_CENTER:
	{
	    Ttk_Box pb = Ttk_AnchorBox(
		b, l->image.width, l->image.height, TK_ANCHOR_CENTER);
	    ImageDraw(&l->image, tkwin, d, pb, state);
	    pb = Ttk_AnchorBox(
		b, l->text.width, l->text.height, TK_ANCHOR_CENTER);
	    TextDraw(&l->text, tkwin, d, pb);
	    break;
	}
	case TTK_COMPOUND_TOP:
	    DrawCompound(l, b, tkwin, d, state, TTK_SIDE_TOP, TTK_SIDE_BOTTOM);
	    break;
	case TTK_COMPOUND_BOTTOM:
	    DrawCompound(l, b, tkwin, d, state, TTK_SIDE_BOTTOM, TTK_SIDE_TOP);
	    break;
	case TTK_COMPOUND_LEFT:
	    DrawCompound(l, b, tkwin, d, state, TTK_SIDE_LEFT, TTK_SIDE_RIGHT);
	    break;
	case TTK_COMPOUND_RIGHT:
	    DrawCompound(l, b, tkwin, d, state, TTK_SIDE_RIGHT, TTK_SIDE_LEFT);
	    break;
    }

    LabelCleanup(l);
}

static Ttk_ElementSpec LabelElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(LabelElement),
    LabelElementOptions,
    LabelElementSize,
    LabelElementDraw
};

/*------------------------------------------------------------------------
 * +++ Initialization.
 */

MODULE_SCOPE
void TtkLabel_Init(Tcl_Interp *interp)
{
    Ttk_Theme theme =  Ttk_GetDefaultTheme(interp);

    Ttk_RegisterElement(interp, theme, "text", &TextElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "image", &ImageElementSpec, NULL);
    Ttk_RegisterElement(interp, theme, "label", &LabelElementSpec, NULL);
}

