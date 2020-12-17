/*
 * tkUnixRFont.c --
 *
 *	Alternate implementation of tkUnixFont.c using Xft.
 *
 * Copyright (c) 2002-2003 Keith Packard
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"
#include "tkFont.h"
#include <X11/Xft/Xft.h>
#include <ctype.h>

#define MAX_CACHED_COLORS 16

typedef struct {
    XftFont *ftFont;
    XftFont *ft0Font;
    FcPattern *source;
    FcCharSet *charset;
    double angle;
} UnixFtFace;

typedef struct {
    XftColor color;
    int next;
} UnixFtColorList;

typedef struct {
    TkFont font;	    	/* Stuff used by generic font package. Must be
				 * first in structure. */
    UnixFtFace *faces;
    int nfaces;
    FcFontSet *fontset;
    FcPattern *pattern;

    Display *display;
    int screen;
    XftDraw *ftDraw;
    int ncolors;
    int firstColor;
    UnixFtColorList colors[MAX_CACHED_COLORS];
} UnixFtFont;

/*
 * Used to describe the current clipping box. Can't be passed normally because
 * the information isn't retrievable from the GC.
 */

typedef struct {
    Region clipRegion;		/* The clipping region, or None. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Package initialization:
 * 	Nothing to do here except register the fact that we're using Xft in
 * 	the TIP 59 configuration database.
 */

#ifndef TCL_CFGVAL_ENCODING
#define TCL_CFGVAL_ENCODING "ascii"
#endif

void
TkpFontPkgInit(
    TkMainInfo *mainPtr)	/* The application being created. */
{
    static const Tcl_Config cfg[] = {
	{ "fontsystem", "xft" },
	{ 0,0 }
    };

    Tcl_RegisterConfig(mainPtr->interp, "tk", cfg, TCL_CFGVAL_ENCODING);
}

static XftFont *
GetFont(
    UnixFtFont *fontPtr,
    FcChar32 ucs4,
    double angle)
{
    int i;

    if (ucs4) {
	for (i = 0; i < fontPtr->nfaces; i++) {
	    FcCharSet *charset = fontPtr->faces[i].charset;

	    if (charset && FcCharSetHasChar(charset, ucs4)) {
		break;
	    }
	}
	if (i == fontPtr->nfaces) {
	    i = 0;
	}
    } else {
	i = 0;
    }
    if ((angle == 0.0 && !fontPtr->faces[i].ft0Font) || (angle != 0.0 &&
	    (!fontPtr->faces[i].ftFont || fontPtr->faces[i].angle != angle))){
	FcPattern *pat = FcFontRenderPrepare(0, fontPtr->pattern,
		fontPtr->faces[i].source);
	double s = sin(angle*PI/180.0), c = cos(angle*PI/180.0);
	FcMatrix mat;
	XftFont *ftFont;

	/*
	 * Initialize the matrix manually so this can compile with HP-UX cc
	 * (which does not allow non-constant structure initializers). [Bug
	 * 2978410]
	 */

	mat.xx = mat.yy = c;
	mat.xy = -(mat.yx = s);

	if (angle != 0.0) {
	    FcPatternAddMatrix(pat, FC_MATRIX, &mat);
	}
	ftFont = XftFontOpenPattern(fontPtr->display, pat);
	if (!ftFont) {
	    /*
	     * The previous call to XftFontOpenPattern() should not fail, but
	     * sometimes does anyway. Usual cause appears to be a
	     * misconfigured fontconfig installation; see [Bug 1090382]. Try a
	     * fallback:
	     */

	    ftFont = XftFontOpen(fontPtr->display, fontPtr->screen,
		    FC_FAMILY, FcTypeString, "sans",
		    FC_SIZE, FcTypeDouble, 12.0,
		    FC_MATRIX, FcTypeMatrix, &mat,
		    NULL);
	}
	if (!ftFont) {
	    /*
	     * The previous call should definitely not fail. Impossible to
	     * proceed at this point.
	     */

	    Tcl_Panic("Cannot find a usable font");
	}

	if (angle == 0.0) {
	    fontPtr->faces[i].ft0Font = ftFont;
	} else {
	    if (fontPtr->faces[i].ftFont) {
		XftFontClose(fontPtr->display, fontPtr->faces[i].ftFont);
	    }
	    fontPtr->faces[i].ftFont = ftFont;
	    fontPtr->faces[i].angle = angle;
	}
    }
    return (angle==0.0? fontPtr->faces[i].ft0Font : fontPtr->faces[i].ftFont);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTkFontAttributes --
 * 	Fill in TkFontAttributes from an XftFont.
 */

static void
GetTkFontAttributes(
    XftFont *ftFont,
    TkFontAttributes *faPtr)
{
    const char *family = "Unknown";
    const char *const *familyPtr = &family;
    int weight, slant, pxsize;
    double size, ptsize;

    (void) XftPatternGetString(ftFont->pattern, XFT_FAMILY, 0, familyPtr);
    if (XftPatternGetDouble(ftFont->pattern, XFT_SIZE, 0,
	    &ptsize) == XftResultMatch) {
	size = ptsize;
    } else if (XftPatternGetDouble(ftFont->pattern, XFT_PIXEL_SIZE, 0,
	    &ptsize) == XftResultMatch) {
	size = -ptsize;
    } else if (XftPatternGetInteger(ftFont->pattern, XFT_PIXEL_SIZE, 0,
	    &pxsize) == XftResultMatch) {
	size = (double)-pxsize;
    } else {
	size = 12.0;
    }
    if (XftPatternGetInteger(ftFont->pattern, XFT_WEIGHT, 0,
	    &weight) != XftResultMatch) {
	weight = XFT_WEIGHT_MEDIUM;
    }
    if (XftPatternGetInteger(ftFont->pattern, XFT_SLANT, 0,
	    &slant) != XftResultMatch) {
	slant = XFT_SLANT_ROMAN;
    }

#if DEBUG_FONTSEL
    printf("family %s size %d weight %d slant %d\n",
	    family, (int)size, weight, slant);
#endif /* DEBUG_FONTSEL */

    faPtr->family = Tk_GetUid(family);
    faPtr->size = size;
    faPtr->weight = (weight > XFT_WEIGHT_MEDIUM) ? TK_FW_BOLD : TK_FW_NORMAL;
    faPtr->slant = (slant > XFT_SLANT_ROMAN) ? TK_FS_ITALIC : TK_FS_ROMAN;
    faPtr->underline = 0;
    faPtr->overstrike = 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTkFontMetrics --
 * 	Fill in TkFontMetrics from an XftFont.
 */

static void
GetTkFontMetrics(
    XftFont *ftFont,
    TkFontMetrics *fmPtr)
{
    int spacing;

    if (XftPatternGetInteger(ftFont->pattern, XFT_SPACING, 0,
	    &spacing) != XftResultMatch) {
	spacing = XFT_PROPORTIONAL;
    }

    fmPtr->ascent = ftFont->ascent;
    fmPtr->descent = ftFont->descent;
    fmPtr->maxWidth = ftFont->max_advance_width;
    fmPtr->fixed = spacing != XFT_PROPORTIONAL;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitFont --
 *
 *	Initializes the fields of a UnixFtFont structure. If fontPtr is NULL,
 *	also allocates a new UnixFtFont.
 *
 * Results:
 * 	On error, frees fontPtr and returns NULL, otherwise returns fontPtr.
 *
 *---------------------------------------------------------------------------
 */

static UnixFtFont *
InitFont(
    Tk_Window tkwin,
    FcPattern *pattern,
    UnixFtFont *fontPtr)
{
    FcFontSet *set;
    FcCharSet *charset;
    FcResult result;
    XftFont *ftFont;
    int i, iWidth;

    if (!fontPtr) {
	fontPtr = ckalloc(sizeof(UnixFtFont));
    }

    FcConfigSubstitute(0, pattern, FcMatchPattern);
    XftDefaultSubstitute(Tk_Display(tkwin), Tk_ScreenNumber(tkwin), pattern);

    /*
     * Generate the list of fonts
     */

    set = FcFontSort(0, pattern, FcTrue, NULL, &result);
    if (!set) {
	ckfree(fontPtr);
	return NULL;
    }

    fontPtr->fontset = set;
    fontPtr->pattern = pattern;
    fontPtr->faces = ckalloc(set->nfont * sizeof(UnixFtFace));
    fontPtr->nfaces = set->nfont;

    /*
     * Fill in information about each returned font
     */

    for (i = 0; i < set->nfont; i++) {
	fontPtr->faces[i].ftFont = 0;
	fontPtr->faces[i].ft0Font = 0;
	fontPtr->faces[i].source = set->fonts[i];
	if (FcPatternGetCharSet(set->fonts[i], FC_CHARSET, 0,
		&charset) == FcResultMatch) {
	    fontPtr->faces[i].charset = FcCharSetCopy(charset);
	} else {
	    fontPtr->faces[i].charset = 0;
	}
	fontPtr->faces[i].angle = 0.0;
    }

    fontPtr->display = Tk_Display(tkwin);
    fontPtr->screen = Tk_ScreenNumber(tkwin);
    fontPtr->ftDraw = 0;
    fontPtr->ncolors = 0;
    fontPtr->firstColor = -1;

    /*
     * Fill in platform-specific fields of TkFont.
     */

    ftFont = GetFont(fontPtr, 0, 0.0);
    fontPtr->font.fid = XLoadFont(Tk_Display(tkwin), "fixed");
    GetTkFontAttributes(ftFont, &fontPtr->font.fa);
    GetTkFontMetrics(ftFont, &fontPtr->font.fm);

    /*
     * Fontconfig can't report any information about the position or thickness
     * of underlines or overstrikes. Thus, we use some defaults that are
     * hacked around from backup defaults in tkUnixFont.c, which are in turn
     * based on recommendations in the X manual. The comments from that file
     * leading to these computations were:
     *
     *	    If the XA_UNDERLINE_POSITION property does not exist, the X manual
     *	    recommends using half the descent.
     *
     *	    If the XA_UNDERLINE_THICKNESS property does not exist, the X
     *	    manual recommends using the width of the stem on a capital letter.
     *	    I don't know of a way to get the stem width of a letter, so guess
     *	    and use 1/3 the width of a capital I.
     *
     * Note that nothing corresponding to *either* property is reported by
     * Fontconfig at all. [Bug 1961455]
     */

    {
	TkFont *fPtr = &fontPtr->font;

	fPtr->underlinePos = fPtr->fm.descent / 2;
	Tk_MeasureChars((Tk_Font) fPtr, "I", 1, -1, 0, &iWidth);
	fPtr->underlineHeight = iWidth / 3;
	if (fPtr->underlineHeight == 0) {
	    fPtr->underlineHeight = 1;
	}
	if (fPtr->underlineHeight + fPtr->underlinePos > fPtr->fm.descent) {
	    fPtr->underlineHeight = fPtr->fm.descent - fPtr->underlinePos;
	    if (fPtr->underlineHeight == 0) {
		fPtr->underlinePos--;
		fPtr->underlineHeight = 1;
	    }
	}
    }

    return fontPtr;
}

static void
FinishedWithFont(
    UnixFtFont *fontPtr)
{
    Display *display = fontPtr->display;
    int i;
    Tk_ErrorHandler handler =
	    Tk_CreateErrorHandler(display, -1, -1, -1, NULL, NULL);

    for (i = 0; i < fontPtr->nfaces; i++) {
	if (fontPtr->faces[i].ftFont) {
	    XftFontClose(fontPtr->display, fontPtr->faces[i].ftFont);
	}
	if (fontPtr->faces[i].ft0Font) {
	    XftFontClose(fontPtr->display, fontPtr->faces[i].ft0Font);
	}
	if (fontPtr->faces[i].charset) {
	    FcCharSetDestroy(fontPtr->faces[i].charset);
	}
    }
    if (fontPtr->faces) {
	ckfree(fontPtr->faces);
    }
    if (fontPtr->pattern) {
	FcPatternDestroy(fontPtr->pattern);
    }
    if (fontPtr->ftDraw) {
	XftDrawDestroy(fontPtr->ftDraw);
    }
    if (fontPtr->font.fid) {
	XUnloadFont(fontPtr->display, fontPtr->font.fid);
    }
    if (fontPtr->fontset) {
	FcFontSetDestroy(fontPtr->fontset);
    }
    Tk_DeleteErrorHandler(handler);
}

TkFont *
TkpGetNativeFont(
    Tk_Window tkwin,		/* For display where font will be used. */
    const char *name)		/* Platform-specific font name. */
{
    UnixFtFont *fontPtr;
    FcPattern *pattern;
#if DEBUG_FONTSEL
    printf("TkpGetNativeFont %s\n", name);
#endif /* DEBUG_FONTSEL */

    pattern = XftXlfdParse(name, FcFalse, FcFalse);
    if (!pattern) {
	return NULL;
    }

    /*
     * Should also try: pattern = FcNameParse(name); but generic/tkFont.c
     * expects TkpGetNativeFont() to only work on XLFD names under Unix.
     */

    fontPtr = InitFont(tkwin, pattern, NULL);
    if (!fontPtr) {
	FcPatternDestroy(pattern);
	return NULL;
    }
    return &fontPtr->font;
}

TkFont *
TkpGetFontFromAttributes(
    TkFont *tkFontPtr,		/* If non-NULL, store the information in this
				 * existing TkFont structure, rather than
				 * allocating a new structure to hold the
				 * font; the existing contents of the font
				 * will be released. If NULL, a new TkFont
				 * structure is allocated. */
    Tk_Window tkwin,		/* For display where font will be used. */
    const TkFontAttributes *faPtr)
				/* Set of attributes to match. */
{
    XftPattern *pattern;
    int weight, slant;
    UnixFtFont *fontPtr;

#if DEBUG_FONTSEL
    printf("TkpGetFontFromAttributes %s-%d %d %d\n", faPtr->family,
	    faPtr->size, faPtr->weight, faPtr->slant);
#endif /* DEBUG_FONTSEL */
    pattern = XftPatternCreate();
    if (faPtr->family) {
	XftPatternAddString(pattern, XFT_FAMILY, faPtr->family);
    }
    if (faPtr->size > 0.0) {
	XftPatternAddDouble(pattern, XFT_SIZE, faPtr->size);
    } else if (faPtr->size < 0.0) {
	XftPatternAddDouble(pattern, XFT_SIZE, TkFontGetPoints(tkwin, faPtr->size));
    } else {
	XftPatternAddDouble(pattern, XFT_SIZE, 12.0);
    }
    switch (faPtr->weight) {
    case TK_FW_NORMAL:
    default:
	weight = XFT_WEIGHT_MEDIUM;
	break;
    case TK_FW_BOLD:
	weight = XFT_WEIGHT_BOLD;
	break;
    }
    XftPatternAddInteger(pattern, XFT_WEIGHT, weight);
    switch (faPtr->slant) {
    case TK_FS_ROMAN:
    default:
	slant = XFT_SLANT_ROMAN;
	break;
    case TK_FS_ITALIC:
	slant = XFT_SLANT_ITALIC;
	break;
    case TK_FS_OBLIQUE:
	slant = XFT_SLANT_OBLIQUE;
	break;
    }
    XftPatternAddInteger(pattern, XFT_SLANT, slant);

    fontPtr = (UnixFtFont *) tkFontPtr;
    if (fontPtr != NULL) {
	FinishedWithFont(fontPtr);
    }
    fontPtr = InitFont(tkwin, pattern, fontPtr);

    /*
     * Hack to work around issues with weird issues with Xft/Xrender
     * connection. For details, see comp.lang.tcl thread starting from
     * <adcc99ed-c73e-4efc-bb5d-e57a57a051e8@l35g2000pra.googlegroups.com>
     */

    if (!fontPtr) {
	XftPatternAddBool(pattern, XFT_RENDER, FcFalse);
	fontPtr = InitFont(tkwin, pattern, fontPtr);
    }

    if (!fontPtr) {
	FcPatternDestroy(pattern);
	return NULL;
    }

    fontPtr->font.fa.underline = faPtr->underline;
    fontPtr->font.fa.overstrike = faPtr->overstrike;
    return &fontPtr->font;
}

void
TkpDeleteFont(
    TkFont *tkFontPtr)		/* Token of font to be deleted. */
{
    UnixFtFont *fontPtr = (UnixFtFont *) tkFontPtr;

    FinishedWithFont(fontPtr);
    /* XXX tkUnixFont.c doesn't free tkFontPtr... */
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetFontFamilies --
 *
 *	Return information about the font families that are available on the
 *	display of the given window.
 *
 * Results:
 *	Modifies interp's result object to hold a list of all the available
 *	font families.
 *
 *---------------------------------------------------------------------------
 */

void
TkpGetFontFamilies(
    Tcl_Interp *interp,		/* Interp to hold result. */
    Tk_Window tkwin)		/* For display to query. */
{
    Tcl_Obj *resultPtr;
    XftFontSet *list;
    int i;

    resultPtr = Tcl_NewListObj(0, NULL);

    list = XftListFonts(Tk_Display(tkwin), Tk_ScreenNumber(tkwin),
		(char *) 0,		/* pattern elements */
		XFT_FAMILY, (char*) 0);	/* fields */
    for (i = 0; i < list->nfont; i++) {
	char *family, **familyPtr = &family;

	if (XftPatternGetString(list->fonts[i], XFT_FAMILY, 0, familyPtr)
		== XftResultMatch) {
	    Tcl_Obj *strPtr = Tcl_NewStringObj(family, -1);

	    Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	}
    }
    XftFontSetDestroy(list);

    Tcl_SetObjResult(interp, resultPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * TkpGetSubFonts --
 *
 *	Called by [testfont subfonts] in the Tk testing package.
 *
 * Results:
 *	Sets interp's result to a list of the faces used by tkfont
 *
 *-------------------------------------------------------------------------
 */

void
TkpGetSubFonts(
    Tcl_Interp *interp,
    Tk_Font tkfont)
{
    Tcl_Obj *objv[3], *listPtr, *resultPtr;
    UnixFtFont *fontPtr = (UnixFtFont *) tkfont;
    FcPattern *pattern;
    const char *family = "Unknown";
    const char *const *familyPtr = &family;
    const char *foundry = "Unknown";
    const char *const *foundryPtr = &foundry;
    const char *encoding = "Unknown";
    const char *const *encodingPtr = &encoding;
    int i;

    resultPtr = Tcl_NewListObj(0, NULL);

    for (i = 0; i < fontPtr->nfaces ; ++i) {
 	pattern = FcFontRenderPrepare(0, fontPtr->pattern,
		fontPtr->faces[i].source);

	XftPatternGetString(pattern, XFT_FAMILY, 0, familyPtr);
	XftPatternGetString(pattern, XFT_FOUNDRY, 0, foundryPtr);
	XftPatternGetString(pattern, XFT_ENCODING, 0, encodingPtr);
	objv[0] = Tcl_NewStringObj(family, -1);
	objv[1] = Tcl_NewStringObj(foundry, -1);
	objv[2] = Tcl_NewStringObj(encoding, -1);
	listPtr = Tcl_NewListObj(3, objv);
	Tcl_ListObjAppendElement(NULL, resultPtr, listPtr);
    }
    Tcl_SetObjResult(interp, resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetFontAttrsForChar --
 *
 *	Retrieve the font attributes of the actual font used to render a given
 *	character.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetFontAttrsForChar(
    Tk_Window tkwin,		/* Window on the font's display */
    Tk_Font tkfont,		/* Font to query */
    int c,			/* Character of interest */
    TkFontAttributes *faPtr)	/* Output: Font attributes */
{
    UnixFtFont *fontPtr = (UnixFtFont *) tkfont;
				/* Structure describing the logical font */
    FcChar32 ucs4 = (FcChar32) c;
				/* UCS-4 character to map */
    XftFont *ftFont = GetFont(fontPtr, ucs4, 0.0);
				/* Actual font used to render the character */

    GetTkFontAttributes(ftFont, faPtr);
    faPtr->underline = fontPtr->font.fa.underline;
    faPtr->overstrike = fontPtr->font.fa.overstrike;
}

int
Tk_MeasureChars(
    Tk_Font tkfont,		/* Font in which characters will be drawn. */
    const char *source,		/* UTF-8 string to be displayed. Need not be
				 * '\0' terminated. */
    int numBytes,		/* Maximum number of bytes to consider from
				 * source string. */
    int maxLength,		/* If >= 0, maxLength specifies the longest
				 * permissible line length in pixels; don't
				 * consider any character that would cross
				 * this x-position. If < 0, then line length
				 * is unbounded and the flags argument is
				 * ignored. */
    int flags,			/* Various flag bits OR-ed together:
				 * TK_PARTIAL_OK means include the last char
				 * which only partially fit on this line.
				 * TK_WHOLE_WORDS means stop on a word
				 * boundary, if possible. TK_AT_LEAST_ONE
				 * means return at least one character even if
				 * no characters fit. */
    int *lengthPtr)		/* Filled with x-location just after the
				 * terminating character. */
{
    UnixFtFont *fontPtr = (UnixFtFont *) tkfont;
    XftFont *ftFont;
    FcChar32 c;
    XGlyphInfo extents;
    int clen, curX, newX, curByte, newByte, sawNonSpace;
    int termByte = 0, termX = 0;
#if DEBUG_FONTSEL
    char string[256];
    int len = 0;
#endif /* DEBUG_FONTSEL */

    curX = 0;
    curByte = 0;
    sawNonSpace = 0;
    while (numBytes > 0) {
	int unichar;

	clen = TkUtfToUniChar(source, &unichar);
	c = (FcChar32) unichar;

	if (clen <= 0) {
	    /*
	     * This can't happen (but see #1185640)
	     */

	    *lengthPtr = curX;
	    return curByte;
	}

	source += clen;
	numBytes -= clen;
	if (c < 256 && isspace(c)) {		/* I18N: ??? */
	    if (sawNonSpace) {
		termByte = curByte;
		termX = curX;
		sawNonSpace = 0;
	    }
	} else {
	    sawNonSpace = 1;
	}

#if DEBUG_FONTSEL
	string[len++] = (char) c;
#endif /* DEBUG_FONTSEL */
	ftFont = GetFont(fontPtr, c, 0.0);

	XftTextExtents32(fontPtr->display, ftFont, &c, 1, &extents);

	newX = curX + extents.xOff;
	newByte = curByte + clen;
	if (maxLength >= 0 && newX > maxLength) {
	    if (flags & TK_PARTIAL_OK ||
		    (flags & TK_AT_LEAST_ONE && curByte == 0)) {
		curX = newX;
		curByte = newByte;
	    } else if (flags & TK_WHOLE_WORDS) {
		if ((flags & TK_AT_LEAST_ONE) && (termX == 0)) {
		    /*
		     * No space was seen before reaching the right
		     * of the allotted maxLength space, i.e. no word
		     * boundary. Return the string that fills the
		     * allotted space, without overfill.
		     * curX and curByte are already the right ones:
		     */
		} else {
		    curX = termX;
		    curByte = termByte;
		}
	    }
	    break;
	}

	curX = newX;
	curByte = newByte;
    }
#if DEBUG_FONTSEL
    string[len] = '\0';
    printf("MeasureChars %s length %d bytes %d\n", string, curX, curByte);
#endif /* DEBUG_FONTSEL */
    *lengthPtr = curX;
    return curByte;
}

int
TkpMeasureCharsInContext(
    Tk_Font tkfont,
    const char *source,
    int numBytes,
    int rangeStart,
    int rangeLength,
    int maxLength,
    int flags,
    int *lengthPtr)
{
    (void) numBytes; /*unused*/

    return Tk_MeasureChars(tkfont, source + rangeStart, rangeLength,
	    maxLength, flags, lengthPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * LookUpColor --
 *
 *	Convert a pixel value to an XftColor.  This can be slow due to the
 * need to call XQueryColor, which involves a server round-trip.  To
 * avoid that, a least-recently-used cache of up to MAX_CACHED_COLORS
 * is kept, in the form of a linked list.  The returned color is moved
 * to the front of the list, so repeatedly asking for the same one
 * should be fast.
 *
 * Results:
 *      A pointer to the XftColor structure for the requested color is
 * returned.
 *
 * Side effects:
 *      The converted color is stored in a cache in the UnixFtFont structure.  The cache
 * can hold at most MAX_CACHED_COLORS colors.  If no more slots are available, the least
 * recently used color is replaced with the new one.
 *----------------------------------------------------------------------
 */

static XftColor *
LookUpColor(Display *display,      /* Display to lookup colors on */
	    UnixFtFont *fontPtr,   /* Font to search for cached colors */
	    unsigned long pixel)   /* Pixel value to translate to XftColor */
{
    int i, last = -1, last2 = -1;
    XColor xcolor;

    for (i = fontPtr->firstColor;
	 i >= 0; last2 = last, last = i, i = fontPtr->colors[i].next) {

	if (pixel == fontPtr->colors[i].color.pixel) {
	    /*
	     * Color found in cache.  Move it to the front of the list and return it.
	     */
	    if (last >= 0) {
		fontPtr->colors[last].next = fontPtr->colors[i].next;
		fontPtr->colors[i].next = fontPtr->firstColor;
		fontPtr->firstColor = i;
	    }

	    return &fontPtr->colors[i].color;
	}
    }

    /*
     * Color wasn't found, so it needs to be added to the cache.
     * If a spare slot is available, it can be put there.  If not, last
     * will now point to the least recently used color, so replace that one.
     */

    if (fontPtr->ncolors < MAX_CACHED_COLORS) {
	last2 = -1;
	last = fontPtr->ncolors++;
    }

    /*
     * Translate the pixel value to a color.  Needs a server round-trip.
     */
    xcolor.pixel = pixel;
    XQueryColor(display, DefaultColormap(display, fontPtr->screen), &xcolor);

    fontPtr->colors[last].color.color.red = xcolor.red;
    fontPtr->colors[last].color.color.green = xcolor.green;
    fontPtr->colors[last].color.color.blue = xcolor.blue;
    fontPtr->colors[last].color.color.alpha = 0xffff;
    fontPtr->colors[last].color.pixel = pixel;

    /*
     * Put at the front of the list.
     */
    if (last2 >= 0) {
	fontPtr->colors[last2].next = fontPtr->colors[last].next;
    }
    fontPtr->colors[last].next = fontPtr->firstColor;
    fontPtr->firstColor = last;

    return &fontPtr->colors[last].color;
}

#define NUM_SPEC    1024

void
Tk_DrawChars(
    Display *display,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    GC gc,			/* Graphics context for drawing characters. */
    Tk_Font tkfont,		/* Font in which characters will be drawn;
				 * must be the same as font used in GC. */
    const char *source,		/* UTF-8 string to be displayed. Need not be
				 * '\0' terminated. All Tk meta-characters
				 * (tabs, control characters, and newlines)
				 * should be stripped out of the string that
				 * is passed to this function. If they are not
				 * stripped out, they will be displayed as
				 * regular printing characters. */
    int numBytes,		/* Number of bytes in string. */
    int x, int y)		/* Coordinates at which to place origin of
				 * string when drawing. */
{
    const int maxCoord = 0x7FFF;/* Xft coordinates are 16 bit values */
    const int minCoord = -maxCoord-1;
    UnixFtFont *fontPtr = (UnixFtFont *) tkfont;
    XGCValues values;
    XftColor *xftcolor;
    int clen, nspec, xStart = x;
    XftGlyphFontSpec specs[NUM_SPEC];
    XGlyphInfo metrics;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (fontPtr->ftDraw == 0) {
#if DEBUG_FONTSEL
	printf("Switch to drawable 0x%x\n", drawable);
#endif /* DEBUG_FONTSEL */
	fontPtr->ftDraw = XftDrawCreate(display, drawable,
		DefaultVisual(display, fontPtr->screen),
		DefaultColormap(display, fontPtr->screen));
    } else {
	Tk_ErrorHandler handler =
		Tk_CreateErrorHandler(display, -1, -1, -1, NULL, NULL);

	XftDrawChange(fontPtr->ftDraw, drawable);
	Tk_DeleteErrorHandler(handler);
    }
    XGetGCValues(display, gc, GCForeground, &values);
    xftcolor = LookUpColor(display, fontPtr, values.foreground);
    if (tsdPtr->clipRegion != None) {
	XftDrawSetClip(fontPtr->ftDraw, tsdPtr->clipRegion);
    }
    nspec = 0;
    while (numBytes > 0) {
	XftFont *ftFont;
	FcChar32 c;

	clen = FcUtf8ToUcs4((FcChar8 *) source, &c, numBytes);
	if (clen <= 0) {
	    /*
	     * This should not happen, but it can.
	     */

	    goto doUnderlineStrikeout;
	}
	source += clen;
	numBytes -= clen;

	ftFont = GetFont(fontPtr, c, 0.0);
	if (ftFont) {
	    specs[nspec].glyph = XftCharIndex(fontPtr->display, ftFont, c);
	    XftGlyphExtents(fontPtr->display, ftFont, &specs[nspec].glyph, 1,
		    &metrics);

	    /*
	     * Draw glyph only when it fits entirely into 16 bit coords.
	     */

	    if (x >= minCoord && y >= minCoord &&
		x <= maxCoord - metrics.width &&
		y <= maxCoord - metrics.height) {
		specs[nspec].font = ftFont;
		specs[nspec].x = x;
		specs[nspec].y = y;
		if (++nspec == NUM_SPEC) {
		    XftDrawGlyphFontSpec(fontPtr->ftDraw, xftcolor,
			    specs, nspec);
		    nspec = 0;
		}
	    }
	    x += metrics.xOff;
	    y += metrics.yOff;
	}
    }
    if (nspec) {
	XftDrawGlyphFontSpec(fontPtr->ftDraw, xftcolor, specs, nspec);
    }

  doUnderlineStrikeout:
    if (tsdPtr->clipRegion != None) {
	XftDrawSetClip(fontPtr->ftDraw, NULL);
    }
    if (fontPtr->font.fa.underline != 0) {
	XFillRectangle(display, drawable, gc, xStart,
		y + fontPtr->font.underlinePos, (unsigned) (x - xStart),
		(unsigned) fontPtr->font.underlineHeight);
    }
    if (fontPtr->font.fa.overstrike != 0) {
	y -= fontPtr->font.fm.descent + (fontPtr->font.fm.ascent) / 10;
	XFillRectangle(display, drawable, gc, xStart, y,
		(unsigned) (x - xStart),
		(unsigned) fontPtr->font.underlineHeight);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkDrawAngledChars --
 *
 *	Draw some characters at an angle. This would be simple code, except
 *	Xft has bugs with cumulative errors in character positioning which are
 *	caused by trying to perform all calculations internally with integers.
 *	So we have to do the work ourselves with floating-point math.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Target drawable is updated.
 *
 *---------------------------------------------------------------------------
 */

void
TkDrawAngledChars(
    Display *display,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    GC gc,			/* Graphics context for drawing characters. */
    Tk_Font tkfont,		/* Font in which characters will be drawn;
				 * must be the same as font used in GC. */
    const char *source,		/* UTF-8 string to be displayed. Need not be
				 * '\0' terminated. All Tk meta-characters
				 * (tabs, control characters, and newlines)
				 * should be stripped out of the string that
				 * is passed to this function. If they are not
				 * stripped out, they will be displayed as
				 * regular printing characters. */
    int numBytes,		/* Number of bytes in string. */
    double x, double y,		/* Coordinates at which to place origin of
				 * string when drawing. */
    double angle)		/* What angle to put text at, in degrees. */
{
    const int maxCoord = 0x7FFF;/* Xft coordinates are 16 bit values */
    const int minCoord = -maxCoord-1;
    UnixFtFont *fontPtr = (UnixFtFont *) tkfont;
    XGCValues values;
    XftColor *xftcolor;
    int xStart = x, yStart = y;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
#ifdef XFT_HAS_FIXED_ROTATED_PLACEMENT
    int clen, nglyph;
    FT_UInt glyphs[NUM_SPEC];
    XGlyphInfo metrics;
    XftFont *currentFtFont;
    int originX, originY;

    if (fontPtr->ftDraw == 0) {
#if DEBUG_FONTSEL
	printf("Switch to drawable 0x%x\n", drawable);
#endif /* DEBUG_FONTSEL */
	fontPtr->ftDraw = XftDrawCreate(display, drawable,
		DefaultVisual(display, fontPtr->screen),
		DefaultColormap(display, fontPtr->screen));
    } else {
	Tk_ErrorHandler handler =
		Tk_CreateErrorHandler(display, -1, -1, -1, NULL, NULL);

	XftDrawChange(fontPtr->ftDraw, drawable);
	Tk_DeleteErrorHandler(handler);
    }

    XGetGCValues(display, gc, GCForeground, &values);
    xftcolor = LookUpColor(display, fontPtr, values.foreground);
    if (tsdPtr->clipRegion != None) {
	XftDrawSetClip(fontPtr->ftDraw, tsdPtr->clipRegion);
    }

    nglyph = 0;
    currentFtFont = NULL;
    originX = originY = 0;		/* lint */

    while (numBytes > 0) {
	XftFont *ftFont;
	FcChar32 c;

	clen = FcUtf8ToUcs4((FcChar8 *) source, &c, numBytes);
	if (clen <= 0) {
	    /*
	     * This should not happen, but it can.
	     */

	    goto doUnderlineStrikeout;
	}
	source += clen;
	numBytes -= clen;

	ftFont = GetFont(fontPtr, c, angle);
	if (!ftFont) {
	    continue;
	}

	if (ftFont != currentFtFont || nglyph == NUM_SPEC) {
	    if (nglyph) {
		/*
		 * We pass multiple glyphs at once to enable the code to
		 * perform better rendering of sub-pixel inter-glyph spacing.
		 * If only the current Xft implementation could make use of
		 * this information... but we'll be ready when it does!
		 */

		XftGlyphExtents(fontPtr->display, currentFtFont, glyphs,
			nglyph, &metrics);
		/*
		 * Draw glyph only when it fits entirely into 16 bit coords.
		 */

		if (x >= minCoord && y >= minCoord &&
		    x <= maxCoord - metrics.width &&
		    y <= maxCoord - metrics.height) {

		    /*
		     * NOTE:
		     * The whole algorithm has a design problem, the choice of
		     * NUM_SPEC is arbitrary, and so the inter-glyph spacing could
		     * look arbitrary. This algorithm has to draw the whole string
		     * at once (or whole blocks with same font), this requires a
		     * dynamic 'glyphs' array. In case of overflow the array has to
		     * be divided until the maximal string will fit. (GC)
                     * Given the resolution of current displays though, this should
                     * not be a huge issue since NUM_SPEC is 1024 and thus able to
                     * cover about 6000 pixels for a 6 pixel wide font (which is
                     * a very small barely readable font)
		     */

		    XftDrawGlyphs(fontPtr->ftDraw, xftcolor, currentFtFont,
			    originX, originY, glyphs, nglyph);
		}
	    }
	    originX = ROUND16(x);
	    originY = ROUND16(y);
	    currentFtFont = ftFont;
	}
	glyphs[nglyph++] = XftCharIndex(fontPtr->display, ftFont, c);
    }
    if (nglyph) {
	XftGlyphExtents(fontPtr->display, currentFtFont, glyphs,
		nglyph, &metrics);

	/*
	 * Draw glyph only when it fits entirely into 16 bit coords.
	 */

	if (x >= minCoord && y >= minCoord &&
	    x <= maxCoord - metrics.width &&
	    y <= maxCoord - metrics.height) {
	    XftDrawGlyphs(fontPtr->ftDraw, xftcolor, currentFtFont,
		    originX, originY, glyphs, nglyph);
	}
    }
#else /* !XFT_HAS_FIXED_ROTATED_PLACEMENT */
    int clen, nspec;
    XftGlyphFontSpec specs[NUM_SPEC];
    XGlyphInfo metrics;
    double sinA = sin(angle * PI/180.0), cosA = cos(angle * PI/180.0);

    if (fontPtr->ftDraw == 0) {
#if DEBUG_FONTSEL
	printf("Switch to drawable 0x%x\n", drawable);
#endif /* DEBUG_FONTSEL */
	fontPtr->ftDraw = XftDrawCreate(display, drawable,
		DefaultVisual(display, fontPtr->screen),
		DefaultColormap(display, fontPtr->screen));
    } else {
	Tk_ErrorHandler handler =
		Tk_CreateErrorHandler(display, -1, -1, -1, NULL, NULL);

	XftDrawChange(fontPtr->ftDraw, drawable);
	Tk_DeleteErrorHandler(handler);
    }
    XGetGCValues(display, gc, GCForeground, &values);
    xftcolor = LookUpColor(display, fontPtr, values.foreground);
    if (tsdPtr->clipRegion != None) {
	XftDrawSetClip(fontPtr->ftDraw, tsdPtr->clipRegion);
    }
    nspec = 0;
    while (numBytes > 0) {
	XftFont *ftFont, *ft0Font;
	FcChar32 c;

	clen = FcUtf8ToUcs4((FcChar8 *) source, &c, numBytes);
	if (clen <= 0) {
	    /*
	     * This should not happen, but it can.
	     */

	    goto doUnderlineStrikeout;
	}
	source += clen;
	numBytes -= clen;

	ftFont = GetFont(fontPtr, c, angle);
	ft0Font = GetFont(fontPtr, c, 0.0);
	if (ftFont && ft0Font) {
	    specs[nspec].glyph = XftCharIndex(fontPtr->display, ftFont, c);
	    XftGlyphExtents(fontPtr->display, ft0Font, &specs[nspec].glyph, 1,
		    &metrics);

	    /*
	     * Draw glyph only when it fits entirely into 16 bit coords.
	     */

	    if (x >= minCoord && y >= minCoord &&
		x <= maxCoord - metrics.width &&
		y <= maxCoord - metrics.height) {
		specs[nspec].font = ftFont;
		specs[nspec].x = ROUND16(x);
		specs[nspec].y = ROUND16(y);
		if (++nspec == NUM_SPEC) {
		    XftDrawGlyphFontSpec(fontPtr->ftDraw, xftcolor,
			    specs, nspec);
		    nspec = 0;
		}
	    }
	    x += metrics.xOff*cosA + metrics.yOff*sinA;
	    y += metrics.yOff*cosA - metrics.xOff*sinA;
	}
    }
    if (nspec) {
	XftDrawGlyphFontSpec(fontPtr->ftDraw, xftcolor, specs, nspec);
    }
#endif /* XFT_HAS_FIXED_ROTATED_PLACEMENT */

  doUnderlineStrikeout:
    if (tsdPtr->clipRegion != None) {
	XftDrawSetClip(fontPtr->ftDraw, NULL);
    }
    if (fontPtr->font.fa.underline || fontPtr->font.fa.overstrike) {
	XPoint points[5];
	double width = (x - xStart) * cosA + (yStart - y) * sinA;
	double barHeight = fontPtr->font.underlineHeight;
	double dy = fontPtr->font.underlinePos;

	if (fontPtr->font.fa.underline != 0) {
	    if (fontPtr->font.underlineHeight == 1) {
		dy++;
	    }
	    points[0].x = xStart + ROUND16(dy*sinA);
	    points[0].y = yStart + ROUND16(dy*cosA);
	    points[1].x = xStart + ROUND16(dy*sinA + width*cosA);
	    points[1].y = yStart + ROUND16(dy*cosA - width*sinA);
	    if (fontPtr->font.underlineHeight == 1) {
		XDrawLines(display, drawable, gc, points, 2, CoordModeOrigin);
	    } else {
		points[2].x = xStart + ROUND16(dy*sinA + width*cosA
			+ barHeight*sinA);
		points[2].y = yStart + ROUND16(dy*cosA - width*sinA
			+ barHeight*cosA);
		points[3].x = xStart + ROUND16(dy*sinA + barHeight*sinA);
		points[3].y = yStart + ROUND16(dy*cosA + barHeight*cosA);
		points[4].x = points[0].x;
		points[4].y = points[0].y;
		XFillPolygon(display, drawable, gc, points, 5, Complex,
			CoordModeOrigin);
		XDrawLines(display, drawable, gc, points, 5, CoordModeOrigin);
	    }
	}
	if (fontPtr->font.fa.overstrike != 0) {
	    dy = -fontPtr->font.fm.descent
		   - (fontPtr->font.fm.ascent) / 10;
	    points[0].x = xStart + ROUND16(dy*sinA);
	    points[0].y = yStart + ROUND16(dy*cosA);
	    points[1].x = xStart + ROUND16(dy*sinA + width*cosA);
	    points[1].y = yStart + ROUND16(dy*cosA - width*sinA);
	    if (fontPtr->font.underlineHeight == 1) {
		XDrawLines(display, drawable, gc, points, 2, CoordModeOrigin);
	    } else {
		points[2].x = xStart + ROUND16(dy*sinA + width*cosA
			+ barHeight*sinA);
		points[2].y = yStart + ROUND16(dy*cosA - width*sinA
			+ barHeight*cosA);
		points[3].x = xStart + ROUND16(dy*sinA + barHeight*sinA);
		points[3].y = yStart + ROUND16(dy*cosA + barHeight*cosA);
		points[4].x = points[0].x;
		points[4].y = points[0].y;
		XFillPolygon(display, drawable, gc, points, 5, Complex,
			CoordModeOrigin);
		XDrawLines(display, drawable, gc, points, 5, CoordModeOrigin);
	    }
	}
    }
}

void
TkUnixSetXftClipRegion(
    TkRegion clipRegion)	/* The clipping region to install. */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *)
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->clipRegion = (Region) clipRegion;
}

/*
 * Local Variables:
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
