/*
 * tkUnixFont.c --
 *
 *	Contains the Unix implementation of the platform-independent font
 *	package interface.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"
#include "tkFont.h"
#include <netinet/in.h>		/* for htons() prototype */
#include <arpa/inet.h>		/* inet_ntoa() */

/*
 * The preferred font encodings.
 */

static const char *const encodingList[] = {
    "iso8859-1", "jis0208", "jis0212", NULL
};

/*
 * The following structure represents a font family. It is assumed that all
 * screen fonts constructed from the same "font family" share certain
 * properties; all screen fonts with the same "font family" point to a shared
 * instance of this structure. The most important shared property is the
 * character existence metrics, used to determine if a screen font can display
 * a given Unicode character.
 *
 * Under Unix, there are three attributes that uniquely identify a "font
 * family": the foundry, face name, and charset.
 */

#define FONTMAP_SHIFT		10

#define FONTMAP_BITSPERPAGE	(1 << FONTMAP_SHIFT)
#define FONTMAP_PAGES		(0x30000 / FONTMAP_BITSPERPAGE)

typedef struct FontFamily {
    struct FontFamily *nextPtr;	/* Next in list of all known font families. */
    int refCount;		/* How many SubFonts are referring to this
				 * FontFamily. When the refCount drops to
				 * zero, this FontFamily may be freed. */
    /*
     * Key.
     */

    Tk_Uid foundry;		/* Foundry key for this FontFamily. */
    Tk_Uid faceName;		/* Face name key for this FontFamily. */
    Tcl_Encoding encoding;	/* Encoding key for this FontFamily. */

    /*
     * Derived properties.
     */

    int isTwoByteFont;		/* 1 if this is a double-byte font, 0
				 * otherwise. */
    char *fontMap[FONTMAP_PAGES];
				/* Two-level sparse table used to determine
				 * quickly if the specified character exists.
				 * As characters are encountered, more pages
				 * in this table are dynamically alloced. The
				 * contents of each page is a bitmask
				 * consisting of FONTMAP_BITSPERPAGE bits,
				 * representing whether this font can be used
				 * to display the given character at the
				 * corresponding bit position. The high bits
				 * of the character are used to pick which
				 * page of the table is used. */
} FontFamily;

/*
 * The following structure encapsulates an individual screen font. A font
 * object is made up of however many SubFonts are necessary to display a
 * stream of multilingual characters.
 */

typedef struct SubFont {
    char **fontMap;		/* Pointer to font map from the FontFamily,
				 * cached here to save a dereference. */
    XFontStruct *fontStructPtr;	/* The specific screen font that will be used
				 * when displaying/measuring chars belonging
				 * to the FontFamily. */
    FontFamily *familyPtr;	/* The FontFamily for this SubFont. */
} SubFont;

/*
 * The following structure represents Unix's implementation of a font object.
 */

#define SUBFONT_SPACE		3
#define BASE_CHARS		256

typedef struct UnixFont {
    TkFont font;		/* Stuff used by generic font package. Must be
				 * first in structure. */
    SubFont staticSubFonts[SUBFONT_SPACE];
				/* Builtin space for a limited number of
				 * SubFonts. */
    int numSubFonts;		/* Length of following array. */
    SubFont *subFontArray;	/* Array of SubFonts that have been loaded in
				 * order to draw/measure all the characters
				 * encountered by this font so far. All fonts
				 * start off with one SubFont initialized by
				 * AllocFont() from the original set of font
				 * attributes. Usually points to
				 * staticSubFonts, but may point to malloced
				 * space if there are lots of SubFonts. */
    SubFont controlSubFont;	/* Font to use to display control-character
				 * expansions. */

    Display *display;		/* Display that owns font. */
    int pixelSize;		/* Original pixel size used when font was
				 * constructed. */
    TkXLFDAttributes xa;	/* Additional attributes that specify the
				 * preferred foundry and encoding to use when
				 * constructing additional SubFonts. */
    int widths[BASE_CHARS];	/* Widths of first 256 chars in the base font,
				 * for handling common case. */
    int underlinePos;		/* Offset from baseline to origin of underline
				 * bar (used when drawing underlined font)
				 * (pixels). */
    int barHeight;		/* Height of underline or overstrike bar (used
				 * when drawing underlined or strikeout font)
				 * (pixels). */
} UnixFont;

/*
 * The following structure and definition is used to keep track of the
 * alternative names for various encodings. Asking for an encoding that
 * matches one of the alias patterns will result in actually getting the
 * encoding by its real name.
 */

typedef struct EncodingAlias {
    const char *realName;	/* The real name of the encoding to load if
				 * the provided name matched the pattern. */
    const char *aliasPattern;	/* Pattern for encoding name, of the form that
				 * is acceptable to Tcl_StringMatch. */
} EncodingAlias;

/*
 * Just some utility structures used for passing around values in helper
 * functions.
 */

typedef struct FontAttributes {
    TkFontAttributes fa;
    TkXLFDAttributes xa;
} FontAttributes;

typedef struct {
    FontFamily *fontFamilyList; /* The list of font families that are
				 * currently loaded. As screen fonts are
				 * loaded, this list grows to hold information
				 * about what characters exist in each font
				 * family. */
    FontFamily controlFamily;	/* FontFamily used to handle control character
				 * expansions. The encoding of this FontFamily
				 * converts UTF-8 to backslashed escape
				 * sequences. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The set of builtin encoding alises to convert the XLFD names for the
 * encodings into the names expected by the Tcl encoding package.
 */

static const EncodingAlias encodingAliases[] = {
    {"gb2312-raw",	"gb2312*"},
    {"big5",		"big5*"},
    {"cns11643-1",	"cns11643*-1"},
    {"cns11643-1",	"cns11643*.1-0"},
    {"cns11643-2",	"cns11643*-2"},
    {"cns11643-2",	"cns11643*.2-0"},
    {"jis0201",		"jisx0201*"},
    {"jis0201",		"jisx0202*"},
    {"jis0208",		"jisc6226*"},
    {"jis0208",		"jisx0208*"},
    {"jis0212",		"jisx0212*"},
    {"tis620",		"tis620*"},
    {"ksc5601",		"ksc5601*"},
    {"dingbats",	"*dingbats"},
    {"ucs-2be",		"iso10646-1"},
    {NULL,		NULL}
};

/*
 * Functions used only in this file.
 */

static void		FontPkgCleanup(ClientData clientData);
static FontFamily *	AllocFontFamily(Display *display,
			    XFontStruct *fontStructPtr, int base);
static SubFont *	CanUseFallback(UnixFont *fontPtr,
			    const char *fallbackName, int ch,
			    SubFont **fixSubFontPtrPtr);
static SubFont *	CanUseFallbackWithAliases(UnixFont *fontPtr,
			    const char *fallbackName, int ch,
			    Tcl_DString *nameTriedPtr,
			    SubFont **fixSubFontPtrPtr);
static int		ControlUtfProc(ClientData clientData, const char *src,
			    int srcLen, int flags, Tcl_EncodingState*statePtr,
			    char *dst, int dstLen, int *srcReadPtr,
			    int *dstWrotePtr, int *dstCharsPtr);
static XFontStruct *	CreateClosestFont(Tk_Window tkwin,
			    const TkFontAttributes *faPtr,
			    const TkXLFDAttributes *xaPtr);
static SubFont *	FindSubFontForChar(UnixFont *fontPtr, int ch,
			    SubFont **fixSubFontPtrPtr);
static void		FontMapInsert(SubFont *subFontPtr, int ch);
static void		FontMapLoadPage(SubFont *subFontPtr, int row);
static int		FontMapLookup(SubFont *subFontPtr, int ch);
static void		FreeFontFamily(FontFamily *afPtr);
static const char *	GetEncodingAlias(const char *name);
static int		GetFontAttributes(Display *display,
			    XFontStruct *fontStructPtr, FontAttributes *faPtr);
static XFontStruct *	GetScreenFont(Display *display,
			    FontAttributes *wantPtr, char **nameList,
			    int bestIdx[], unsigned bestScore[]);
static XFontStruct *	GetSystemFont(Display *display);
static int		IdentifySymbolEncodings(FontAttributes *faPtr);
static void		InitFont(Tk_Window tkwin, XFontStruct *fontStructPtr,
			    UnixFont *fontPtr);
static void		InitSubFont(Display *display,
			    XFontStruct *fontStructPtr, int base,
			    SubFont *subFontPtr);
static char **		ListFonts(Display *display, const char *faceName,
			    int *numNamesPtr);
static char **		ListFontOrAlias(Display *display, const char*faceName,
			    int *numNamesPtr);
static unsigned		RankAttributes(FontAttributes *wantPtr,
			    FontAttributes *gotPtr);
static void		ReleaseFont(UnixFont *fontPtr);
static void		ReleaseSubFont(Display *display, SubFont *subFontPtr);
static int		SeenName(const char *name, Tcl_DString *dsPtr);
static int		Ucs2beToUtfProc(ClientData clientData, const char*src,
			    int srcLen, int flags, Tcl_EncodingState*statePtr,
			    char *dst, int dstLen, int *srcReadPtr,
			    int *dstWrotePtr, int *dstCharsPtr);
static int		UtfToUcs2beProc(ClientData clientData, const char*src,
			    int srcLen, int flags, Tcl_EncodingState*statePtr,
			    char *dst, int dstLen, int *srcReadPtr,
			    int *dstWrotePtr, int *dstCharsPtr);

/*
 *-------------------------------------------------------------------------
 *
 * FontPkgCleanup --
 *
 *	This function is called when an application is created. It initializes
 *	all the structures that are used by the platform-dependent code on a
 *	per application basis.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Releases thread-specific resources used by font pkg.
 *
 *-------------------------------------------------------------------------
 */

static void
FontPkgCleanup(
    ClientData clientData)
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (tsdPtr->controlFamily.encoding != NULL) {
	FontFamily *familyPtr = &tsdPtr->controlFamily;
	int i;

	Tcl_FreeEncoding(familyPtr->encoding);
	for (i = 0; i < FONTMAP_PAGES; i++) {
	    if (familyPtr->fontMap[i] != NULL) {
		ckfree(familyPtr->fontMap[i]);
	    }
	}
	tsdPtr->controlFamily.encoding = NULL;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * TkpFontPkgInit --
 *
 *	This function is called when an application is created. It initializes
 *	all the structures that are used by the platform-dependent code on a
 *	per application basis.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

void
TkpFontPkgInit(
    TkMainInfo *mainPtr)	/* The application being created. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    SubFont dummy;
    int i;
    Tcl_Encoding ucs2;

    if (tsdPtr->controlFamily.encoding == NULL) {

	Tcl_EncodingType type = {"X11ControlChars", ControlUtfProc, ControlUtfProc, NULL, NULL, 0};
	tsdPtr->controlFamily.refCount = 2;
	tsdPtr->controlFamily.encoding = Tcl_CreateEncoding(&type);
	tsdPtr->controlFamily.isTwoByteFont = 0;

	dummy.familyPtr = &tsdPtr->controlFamily;
	dummy.fontMap = tsdPtr->controlFamily.fontMap;
	for (i = 0x00; i < 0x20; i++) {
	    FontMapInsert(&dummy, i);
	    FontMapInsert(&dummy, i + 0x80);
	}

	/*
	 * UCS-2BE is unicode (UCS-2) in big-endian format. Define this if
	 * if it doesn't exist yet. It is used in iso10646 fonts.
	 */

	ucs2 = Tcl_GetEncoding(NULL, "ucs-2be");
	if (ucs2 == NULL) {
	    Tcl_EncodingType ucs2type = {"ucs-2be", Ucs2beToUtfProc, UtfToUcs2beProc, NULL, NULL, 2};
	    Tcl_CreateEncoding(&ucs2type);
	} else {
	    Tcl_FreeEncoding(ucs2);
	}
	Tcl_CreateThreadExitHandler(FontPkgCleanup, NULL);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * ControlUtfProc --
 *
 *	Convert from UTF-8 into the ASCII expansion of a control character.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
ControlUtfProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd;
    char *dstStart, *dstEnd;
    int ch, result;
    static char hexChars[] = "0123456789abcdef";
    static char mapChars[] = {
	0, 0, 0, 0, 0, 0, 0,
	'a', 'b', 't', 'n', 'v', 'f', 'r'
    };

    result = TCL_OK;

    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - 6;

    for ( ; src < srcEnd; ) {
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	src += TkUtfToUniChar(src, &ch);
	dst[0] = '\\';
	if (((size_t)ch < sizeof(mapChars)) && (mapChars[ch] != 0)) {
	    dst[1] = mapChars[ch];
	    dst += 2;
	} else if ((size_t)ch < 256) {
	    dst[1] = 'x';
	    dst[2] = hexChars[(ch >> 4) & 0xf];
	    dst[3] = hexChars[ch & 0xf];
	    dst += 4;
	} else if ((size_t)ch < 0x10000) {
	    dst[1] = 'u';
	    dst[2] = hexChars[(ch >> 12) & 0xf];
	    dst[3] = hexChars[(ch >> 8) & 0xf];
	    dst[4] = hexChars[(ch >> 4) & 0xf];
	    dst[5] = hexChars[ch & 0xf];
	    dst += 6;
	} else {
	    /* TODO we can do better here */
	    dst[1] = 'u';
	    dst[2] = 'f';
	    dst[3] = 'f';
	    dst[4] = 'f';
	    dst[5] = 'd';
	    dst += 6;
	}
    }
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = dst - dstStart;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * Ucs2beToUtfProc --
 *
 *	Convert from UCS-2BE (big-endian 16-bit Unicode) to UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
Ucs2beToUtfProc(
    ClientData clientData,	/* Not used. */
    const char *src,		/* Source string in Unicode. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd;
    char *dstEnd, *dstStart;
    int result, numChars;

    result = TCL_OK;

    /* check alignment with ucs-2 (2 == sizeof(UCS-2)) */
    if ((srcLen % 2) != 0) {
	result = TCL_CONVERT_MULTIBYTE;
	srcLen--;
    }
    /* If last code point is a high surrogate, we cannot handle that yet */
    if ((srcLen >= 2) && ((src[srcLen - 2] & 0xFC) == 0xD8)) {
	result = TCL_CONVERT_MULTIBYTE;
	srcLen -= 2;
    }

    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    for (numChars = 0; src < srcEnd; numChars++) {
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}

	/*
	 * Need to swap byte-order on little-endian machines (x86) for
	 * UCS-2BE. We know this is an LE->BE swap.
	 */

	dst += TkUniCharToUtf(htons(*((short *)src)), dst);
	src += 2 /* sizeof(UCS-2) */;
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfToUcs2beProc --
 *
 *	Convert from UTF-8 to UCS-2BE (fixed 2-byte encoding).
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
UtfToUcs2beProc(
    ClientData clientData,	/* TableEncodingData that specifies
				 * encoding. */
    const char *src,		/* Source string in UTF-8. */
    int srcLen,			/* Source string length in bytes. */
    int flags,			/* Conversion control flags. */
    Tcl_EncodingState *statePtr,/* Place for conversion routine to store state
				 * information used during a piecewise
				 * conversion. Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst,			/* Output buffer in which converted string is
				 * stored. */
    int dstLen,			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr,		/* Filled with the number of bytes from the
				 * source string that were converted. This may
				 * be less than the original source length if
				 * there was a problem converting some source
				 * characters. */
    int *dstWrotePtr,		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr)		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    const char *srcStart, *srcEnd, *srcClose, *dstStart, *dstEnd;
    int result, numChars;
    Tcl_UniChar *chPtr = (Tcl_UniChar *)statePtr;

    if (flags & TCL_ENCODING_START) {
	*statePtr = 0;
    }

    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if (!(flags & TCL_ENCODING_END)) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - 2 /* sizeof(UCS-2) */;

    result = TCL_OK;
    for (numChars = 0; src < srcEnd; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */
	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	src += Tcl_UtfToUniChar(src, chPtr);

	/*
	 * Ensure big-endianness (store big bits first).
	 * XXX: This hard-codes the assumed size of Tcl_UniChar as 2. Make
	 * sure to work in char* for Tcl_UtfToUniChar alignment. [Bug 1122671]
	 */


	*dst++ = (char)(*chPtr >> 8);
	*dst++ = (char)*chPtr;
    }
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetNativeFont --
 *
 *	Map a platform-specific native font name to a TkFont.
 *
 * Results:
 *	The return value is a pointer to a TkFont that represents the native
 *	font. If a native font by the given name could not be found, the
 *	return value is NULL.
 *
 *	Every call to this function returns a new TkFont structure, even if
 *	the name has already been seen before. The caller should call
 *	TkpDeleteFont() when the font is no longer needed.
 *
 *	The caller is responsible for initializing the memory associated with
 *	the generic TkFont when this function returns and releasing the
 *	contents of the generic TkFont before calling TkpDeleteFont().
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

TkFont *
TkpGetNativeFont(
    Tk_Window tkwin,		/* For display where font will be used. */
    const char *name)		/* Platform-specific font name. */
{
    UnixFont *fontPtr;
    XFontStruct *fontStructPtr;
    FontAttributes fa;
    const char *p;
    int hasSpace, dashes, hasWild;

    /*
     * The behavior of X when given a name that isn't an XLFD is unspecified.
     * For example, Exceed 6 returns a valid font for any random string. This
     * is awkward since system names have higher priority than the other Tk
     * font syntaxes. So, we need to perform a quick sanity check on the name
     * and fail if it looks suspicious. We fail if the name:
     *     - contains a space immediately before a dash
     *	   - contains a space, but no '*' characters and fewer than 14 dashes
     */

    hasSpace = dashes = hasWild = 0;
    for (p = name; *p != '\0'; p++) {
	if (*p == ' ') {
	    if (p[1] == '-') {
		return NULL;
	    }
	    hasSpace = 1;
	} else if (*p == '-') {
	    dashes++;
	} else if (*p == '*') {
	    hasWild = 1;
	}
    }
    if ((dashes < 14) && !hasWild && hasSpace) {
	return NULL;
    }

    fontStructPtr = XLoadQueryFont(Tk_Display(tkwin), name);
    if (fontStructPtr == NULL) {
	/*
	 * Handle all names that look like XLFDs here. Otherwise, when
	 * TkpGetFontFromAttributes is called from generic code, any foundry
	 * or encoding information specified in the XLFD will have been parsed
	 * out and lost. But make sure we don't have an "-option value" string
	 * since TkFontParseXLFD would return a false success when attempting
	 * to parse it.
	 */

	if (name[0] == '-') {
	    if (name[1] != '*') {
		char *dash;

		dash = strchr(name + 1, '-');
		if ((dash == NULL) || (isspace(UCHAR(dash[-1])))) {
		    return NULL;
		}
	    }
	} else if (name[0] != '*') {
	    return NULL;
	}
	if (TkFontParseXLFD(name, &fa.fa, &fa.xa) != TCL_OK) {
	    return NULL;
	}
	fontStructPtr = CreateClosestFont(tkwin, &fa.fa, &fa.xa);
    }
    fontPtr = ckalloc(sizeof(UnixFont));
    InitFont(tkwin, fontStructPtr, fontPtr);

    return (TkFont *) fontPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpGetFontFromAttributes --
 *
 *	Given a desired set of attributes for a font, find a font with the
 *	closest matching attributes.
 *
 * Results:
 *	The return value is a pointer to a TkFont that represents the font
 *	with the desired attributes. If a font with the desired attributes
 *	could not be constructed, some other font will be substituted
 *	automatically.
 *
 *	Every call to this function returns a new TkFont structure, even if
 *	the specified attributes have already been seen before. The caller
 *	should call TkpDeleteFont() to free the platform- specific data when
 *	the font is no longer needed.
 *
 *	The caller is responsible for initializing the memory associated with
 *	the generic TkFont when this function returns and releasing the
 *	contents of the generic TkFont before calling TkpDeleteFont().
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

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
    UnixFont *fontPtr;
    TkXLFDAttributes xa;
    XFontStruct *fontStructPtr;

    TkInitXLFDAttributes(&xa);
    fontStructPtr = CreateClosestFont(tkwin, faPtr, &xa);

    fontPtr = (UnixFont *) tkFontPtr;
    if (fontPtr == NULL) {
	fontPtr = ckalloc(sizeof(UnixFont));
    } else {
	ReleaseFont(fontPtr);
    }
    InitFont(tkwin, fontStructPtr, fontPtr);

    fontPtr->font.fa.underline = faPtr->underline;
    fontPtr->font.fa.overstrike = faPtr->overstrike;

    return (TkFont *) fontPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpDeleteFont --
 *
 *	Called to release a font allocated by TkpGetNativeFont() or
 *	TkpGetFontFromAttributes(). The caller should have already released
 *	the fields of the TkFont that are used exclusively by the generic
 *	TkFont code.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	TkFont is deallocated.
 *
 *---------------------------------------------------------------------------
 */

void
TkpDeleteFont(
    TkFont *tkFontPtr)		/* Token of font to be deleted. */
{
    UnixFont *fontPtr = (UnixFont *) tkFontPtr;

    ReleaseFont(fontPtr);
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
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TkpGetFontFamilies(
    Tcl_Interp *interp,		/* Interp to hold result. */
    Tk_Window tkwin)		/* For display to query. */
{
    int i, new, numNames;
    char *family, **nameList;
    Tcl_HashTable familyTable;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;
    Tcl_Obj *resultPtr, *strPtr;

    Tcl_InitHashTable(&familyTable, TCL_STRING_KEYS);
    nameList = ListFonts(Tk_Display(tkwin), "*", &numNames);
    for (i = 0; i < numNames; i++) {
	char *familyEnd;

	family = strchr(nameList[i] + 1, '-');
	if (family == NULL) {
	    /*
	     * Apparently, sometimes ListFonts() can return a font name with
	     * zero or one '-' character in it. This is probably indicative of
	     * a server misconfiguration, but crashing because of it is a very
	     * bad idea anyway. [Bug 1475865]
	     */

	    continue;
	}
	family++;			/* Advance to char after '-'. */
	familyEnd = strchr(family, '-');
	if (familyEnd == NULL) {
	    continue;			/* See comment above. */
	}
	*familyEnd = '\0';
	Tcl_CreateHashEntry(&familyTable, family, &new);
    }
    XFreeFontNames(nameList);

    hPtr = Tcl_FirstHashEntry(&familyTable, &search);
    resultPtr = Tcl_NewObj();
    while (hPtr != NULL) {
	strPtr = Tcl_NewStringObj(Tcl_GetHashKey(&familyTable, hPtr), -1);
	Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_SetObjResult(interp, resultPtr);

    Tcl_DeleteHashTable(&familyTable);
}

/*
 *-------------------------------------------------------------------------
 *
 * TkpGetSubFonts --
 *
 *	A function used by the testing package for querying the actual screen
 *	fonts that make up a font object.
 *
 * Results:
 *	Modifies interp's result object to hold a list containing the names of
 *	the screen fonts that make up the given font object.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

void
TkpGetSubFonts(
    Tcl_Interp *interp,
    Tk_Font tkfont)
{
    int i;
    Tcl_Obj *objv[3], *resultPtr, *listPtr;
    UnixFont *fontPtr;
    FontFamily *familyPtr;

    resultPtr = Tcl_NewObj();
    fontPtr = (UnixFont *) tkfont;
    for (i = 0; i < fontPtr->numSubFonts; i++) {
	familyPtr = fontPtr->subFontArray[i].familyPtr;
	objv[0] = Tcl_NewStringObj(familyPtr->faceName, -1);
	objv[1] = Tcl_NewStringObj(familyPtr->foundry, -1);
	objv[2] = Tcl_NewStringObj(
		Tcl_GetEncodingName(familyPtr->encoding), -1);
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
 * Results:
 *	None.
 *
 * Side effects:
 *	The font attributes are stored in *faPtr.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetFontAttrsForChar(
    Tk_Window tkwin,		/* Window on the font's display */
    Tk_Font tkfont,		/* Font to query */
    int c,         		/* Character of interest */
    TkFontAttributes *faPtr)	/* Output: Font attributes */
{
    FontAttributes atts;
    UnixFont *fontPtr = (UnixFont *) tkfont;
				/* Structure describing the logical font */
    SubFont *lastSubFontPtr = &fontPtr->subFontArray[0];
				/* Pointer to subfont array in case
				 * FindSubFontForChar needs to fix up the
				 * memory allocation */
    SubFont *thisSubFontPtr = FindSubFontForChar(fontPtr, c, &lastSubFontPtr);
				/* Pointer to the subfont to use for the given
				 * character */

    GetFontAttributes(Tk_Display(tkwin), thisSubFontPtr->fontStructPtr, &atts);
    *faPtr = atts.fa;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_MeasureChars --
 *
 *	Determine the number of characters from the string that will fit in
 *	the given horizontal span. The measurement is done under the
 *	assumption that Tk_DrawChars() will be used to actually display the
 *	characters.
 *
 * Results:
 *	The return value is the number of bytes from source that fit into the
 *	span that extends from 0 to maxLength. *lengthPtr is filled with the
 *	x-coordinate of the right edge of the last character that did fit.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

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
    UnixFont *fontPtr;
    SubFont *lastSubFontPtr;
    int curX, curByte, ch;

    /*
     * Unix does not use kerning or fractional character widths when
     * displaying text on the screen. So that means we can safely measure
     * individual characters or spans of characters and add up the widths w/o
     * any "off-by-one-pixel" errors.
     */

    fontPtr = (UnixFont *) tkfont;

    lastSubFontPtr = &fontPtr->subFontArray[0];

    if (numBytes == 0) {
	curX = 0;
	curByte = 0;
    } else if (maxLength < 0) {
	const char *p, *end, *next;
	SubFont *thisSubFontPtr;
	FontFamily *familyPtr;
	Tcl_DString runString;

	/*
	 * A three step process:
	 * 1. Find a contiguous range of characters that can all be
	 *    represented by a single screen font.
	 * 2. Convert those chars to the encoding of that font.
	 * 3. Measure converted chars.
	 */

	curX = 0;
	end = source + numBytes;
	for (p = source; p < end; ) {
	    next = p + TkUtfToUniChar(p, &ch);
	    thisSubFontPtr = FindSubFontForChar(fontPtr, ch, &lastSubFontPtr);
	    if (thisSubFontPtr != lastSubFontPtr) {
		familyPtr = lastSubFontPtr->familyPtr;
		Tcl_UtfToExternalDString(familyPtr->encoding, source,
			p - source, &runString);
		if (familyPtr->isTwoByteFont) {
		    curX += XTextWidth16(lastSubFontPtr->fontStructPtr,
			    (XChar2b *) Tcl_DStringValue(&runString),
			    Tcl_DStringLength(&runString) / 2);
		} else {
		    curX += XTextWidth(lastSubFontPtr->fontStructPtr,
			    Tcl_DStringValue(&runString),
			    Tcl_DStringLength(&runString));
		}
		Tcl_DStringFree(&runString);
		lastSubFontPtr = thisSubFontPtr;
		source = p;
	    }
	    p = next;
	}
	familyPtr = lastSubFontPtr->familyPtr;
	Tcl_UtfToExternalDString(familyPtr->encoding, source, p - source,
		&runString);
	if (familyPtr->isTwoByteFont) {
	    curX += XTextWidth16(lastSubFontPtr->fontStructPtr,
		    (XChar2b *) Tcl_DStringValue(&runString),
		    Tcl_DStringLength(&runString) >> 1);
	} else {
	    curX += XTextWidth(lastSubFontPtr->fontStructPtr,
		    Tcl_DStringValue(&runString),
		    Tcl_DStringLength(&runString));
	}
	Tcl_DStringFree(&runString);
	curByte = numBytes;
    } else {
	const char *p, *end, *next, *term;
	int newX, termX, sawNonSpace, dstWrote;
	FontFamily *familyPtr;
	XChar2b buf[8];

	/*
	 * How many chars will fit in the space allotted? This first version
	 * may be inefficient because it measures every character
	 * individually.
	 */

	next = source + TkUtfToUniChar(source, &ch);
	newX = curX = termX = 0;

	term = source;
	end = source + numBytes;

	sawNonSpace = (ch > 255) || !isspace(ch);
	familyPtr = lastSubFontPtr->familyPtr;
	for (p = source; ; ) {
	    if ((ch < BASE_CHARS) && (fontPtr->widths[ch] != 0)) {
		newX += fontPtr->widths[ch];
	    } else {
		lastSubFontPtr = FindSubFontForChar(fontPtr, ch, NULL);
		familyPtr = lastSubFontPtr->familyPtr;
		Tcl_UtfToExternal(NULL, familyPtr->encoding, p, next - p, 0, NULL,
			(char *)&buf[0].byte1, sizeof(buf), NULL, &dstWrote, NULL);
		if (familyPtr->isTwoByteFont) {
		    newX += XTextWidth16(lastSubFontPtr->fontStructPtr,
			    buf, dstWrote >> 1);
		} else {
		    newX += XTextWidth(lastSubFontPtr->fontStructPtr,
			    (char *)&buf[0].byte1, dstWrote);
		}
	    }
	    if (newX > maxLength) {
		break;
	    }
	    curX = newX;
	    p = next;
	    if (p >= end) {
		term = end;
		termX = curX;
		break;
	    }

	    next += TkUtfToUniChar(next, &ch);
	    if ((ch < 256) && isspace(ch)) {
		if (sawNonSpace) {
		    term = p;
		    termX = curX;
		    sawNonSpace = 0;
		}
	    } else {
		sawNonSpace = 1;
	    }
	}

	/*
	 * P points to the first character that doesn't fit in the desired
	 * span. Use the flags to figure out what to return.
	 */

	if ((flags & TK_PARTIAL_OK) && (p < end) && (curX < maxLength)) {
	    /*
	     * Include the first character that didn't quite fit in the
	     * desired span. The width returned will include the width of that
	     * extra character.
	     */

	    curX = newX;
	    p += TkUtfToUniChar(p, &ch);
	}
	if ((flags & TK_AT_LEAST_ONE) && (term == source) && (p < end)) {
	    term = p;
	    termX = curX;
	    if (term == source) {
		term += TkUtfToUniChar(term, &ch);
		termX = newX;
	    }
	} else if ((p >= end) || !(flags & TK_WHOLE_WORDS)) {
	    term = p;
	    termX = curX;
	}

	curX = termX;
	curByte = term - source;
    }

    *lengthPtr = curX;
    return curByte;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpMeasureCharsInContext --
 *
 *	Determine the number of bytes from the string that will fit in the
 *	given horizontal span. The measurement is done under the assumption
 *	that TkpDrawCharsInContext() will be used to actually display the
 *	characters.
 *
 *	This one is almost the same as Tk_MeasureChars(), but with access to
 *	all the characters on the line for context. On X11 this context isn't
 *	consulted, so we just call Tk_MeasureChars().
 *
 * Results:
 *	The return value is the number of bytes from source that fit into the
 *	span that extends from 0 to maxLength. *lengthPtr is filled with the
 *	x-coordinate of the right edge of the last character that did fit.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TkpMeasureCharsInContext(
    Tk_Font tkfont,		/* Font in which characters will be drawn. */
    const char *source,		/* UTF-8 string to be displayed. Need not be
				 * '\0' terminated. */
    int numBytes,		/* Maximum number of bytes to consider from
				 * source string in all. */
    int rangeStart,		/* Index of first byte to measure. */
    int rangeLength,		/* Length of range to measure in bytes. */
    int maxLength,		/* If >= 0, maxLength specifies the longest
				 * permissible line length; don't consider any
				 * character that would cross this x-position.
				 * If < 0, then line length is unbounded and
				 * the flags argument is ignored. */
    int flags,			/* Various flag bits OR-ed together:
				 * TK_PARTIAL_OK means include the last char
				 * which only partially fit on this line.
				 * TK_WHOLE_WORDS means stop on a word
				 * boundary, if possible. TK_AT_LEAST_ONE
				 * means return at least one character even if
				 * no characters fit. TK_ISOLATE_END means
				 * that the last character should not be
				 * considered in context with the rest of the
				 * string (used for breaking lines). */
    int *lengthPtr)		/* Filled with x-location just after the
				 * terminating character. */
{
    (void) numBytes; /*unused*/
    return Tk_MeasureChars(tkfont, source + rangeStart, rangeLength,
	    maxLength, flags, lengthPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tk_DrawChars --
 *
 *	Draw a string of characters on the screen. Tk_DrawChars() expands
 *	control characters that occur in the string to \xNN sequences.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */

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
    UnixFont *fontPtr = (UnixFont *) tkfont;
    SubFont *thisSubFontPtr, *lastSubFontPtr;
    Tcl_DString runString;
    const char *p, *end, *next;
    int xStart, needWidth, window_width, do_width, ch;
    FontFamily *familyPtr;
#ifdef TK_DRAW_CHAR_XWINDOW_CHECK
    int rx, ry;
    unsigned width, height, border_width, depth;
    Drawable root;
#endif

    lastSubFontPtr = &fontPtr->subFontArray[0];
    xStart = x;

#ifdef TK_DRAW_CHAR_XWINDOW_CHECK
    /*
     * Get the window width so we can abort drawing outside of the window
     */

    if (XGetGeometry(display, drawable, &root, &rx, &ry, &width, &height,
	    &border_width, &depth) == False) {
	window_width = INT_MAX;
    } else {
	window_width = width;
    }
#else
    /*
     * This is used by default until we find a solution that doesn't do a
     * round-trip to the X server (needed to get Tk cached window width).
     */

    window_width = 32768;
#endif

    end = source + numBytes;
    needWidth = fontPtr->font.fa.underline + fontPtr->font.fa.overstrike;
    for (p = source; p <= end; ) {
	if (p < end) {
	    next = p + TkUtfToUniChar(p, &ch);
	    thisSubFontPtr = FindSubFontForChar(fontPtr, ch, &lastSubFontPtr);
	} else {
	    next = p + 1;
	    thisSubFontPtr = lastSubFontPtr;
	}
	if ((thisSubFontPtr != lastSubFontPtr)
		|| (p == end) || (p-source > 200)) {
	    if (p > source) {
		do_width = (needWidth || (p != end)) ? 1 : 0;
		familyPtr = lastSubFontPtr->familyPtr;

		Tcl_UtfToExternalDString(familyPtr->encoding, source,
			p - source, &runString);
		if (familyPtr->isTwoByteFont) {
		    XDrawString16(display, drawable, gc, x, y,
			    (XChar2b *) Tcl_DStringValue(&runString),
			    Tcl_DStringLength(&runString) / 2);
		    if (do_width) {
			x += XTextWidth16(lastSubFontPtr->fontStructPtr,
				(XChar2b *) Tcl_DStringValue(&runString),
				Tcl_DStringLength(&runString) / 2);
		    }
		} else {
		    XDrawString(display, drawable, gc, x, y,
			    Tcl_DStringValue(&runString),
			    Tcl_DStringLength(&runString));
		    if (do_width) {
			x += XTextWidth(lastSubFontPtr->fontStructPtr,
				Tcl_DStringValue(&runString),
				Tcl_DStringLength(&runString));
		    }
		}
		Tcl_DStringFree(&runString);
	    }
	    lastSubFontPtr = thisSubFontPtr;
	    source = p;
	    XSetFont(display, gc, lastSubFontPtr->fontStructPtr->fid);
	    if (x > window_width) {
		break;
	    }
	}
	p = next;
    }

    if (lastSubFontPtr != &fontPtr->subFontArray[0]) {
	XSetFont(display, gc, fontPtr->subFontArray[0].fontStructPtr->fid);
    }

    if (fontPtr->font.fa.underline != 0) {
	XFillRectangle(display, drawable, gc, xStart,
		y + fontPtr->underlinePos,
		(unsigned) (x - xStart), (unsigned) fontPtr->barHeight);
    }
    if (fontPtr->font.fa.overstrike != 0) {
	y -= fontPtr->font.fm.descent + (fontPtr->font.fm.ascent) / 10;
	XFillRectangle(display, drawable, gc, xStart, y,
		(unsigned) (x - xStart), (unsigned) fontPtr->barHeight);
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TkpDrawCharsInContext --
 *
 *	Draw a string of characters on the screen like Tk_DrawChars(), but
 *	with access to all the characters on the line for context. On X11 this
 *	context isn't consulted, so we just call Tk_DrawChars().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Information gets drawn on the screen.
 *
 *---------------------------------------------------------------------------
 */

void
TkpDrawCharsInContext(
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
    int rangeStart,		/* Index of first byte to draw. */
    int rangeLength,		/* Length of range to draw in bytes. */
    int x, int y)		/* Coordinates at which to place origin of the
				 * whole (not just the range) string when
				 * drawing. */
{
    int widthUntilStart;

    (void) numBytes; /*unused*/

    Tk_MeasureChars(tkfont, source, rangeStart, -1, 0, &widthUntilStart);
    Tk_DrawChars(display, drawable, gc, tkfont, source + rangeStart,
	    rangeLength, x+widthUntilStart, y);
}

/*
 *-------------------------------------------------------------------------
 *
 * CreateClosestFont --
 *
 *	Helper for TkpGetNativeFont() and TkpGetFontFromAttributes(). Given a
 *	set of font attributes, construct a close XFontStruct. If requested
 *	face name is not available, automatically substitutes an alias for
 *	requested face name. If encoding is not specified (or the requested
 *	one is not available), automatically chooses another encoding from the
 *	list of preferred encodings. If the foundry is not specified (or is
 *	not available) automatically prefers "adobe" foundry. For all other
 *	attributes, if the requested value was not available, the appropriate
 *	"close" value will be used.
 *
 * Results:
 *	Return value is the XFontStruct that best matched the requested
 *	attributes. The return value is never NULL; some font will always be
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static XFontStruct *
CreateClosestFont(
    Tk_Window tkwin,		/* For display where font will be used. */
    const TkFontAttributes *faPtr,
				/* Set of generic attributes to match. */
    const TkXLFDAttributes *xaPtr)
				/* Set of X-specific attributes to match. */
{
    FontAttributes want;
    char **nameList;
    int numNames, nameIdx, bestIdx[2];
    Display *display;
    XFontStruct *fontStructPtr;
    unsigned bestScore[2];

    want.fa = *faPtr;
    want.xa = *xaPtr;

    if (want.xa.foundry == NULL) {
	want.xa.foundry = Tk_GetUid("adobe");
    }
    if (want.fa.family == NULL) {
	want.fa.family = Tk_GetUid("fixed");
    }
    want.fa.size = -TkFontGetPixels(tkwin, faPtr->size);
    if (want.xa.charset == NULL || *want.xa.charset == '\0') {
	want.xa.charset = Tk_GetUid("iso8859-1");	/* locale. */
    }

    display = Tk_Display(tkwin);

    /*
     * Algorithm to get the closest font to the name requested.
     *
     * try fontname
     * try all aliases for fontname
     * foreach fallback for fontname
     *	    try the fallback
     *	    try all aliases for the fallback
     */

    nameList = ListFontOrAlias(display, want.fa.family, &numNames);
    if (numNames == 0) {
	const char *const *const *fontFallbacks;
	int i, j;
	const char *fallback;

	fontFallbacks = TkFontGetFallbacks();
	for (i = 0; fontFallbacks[i] != NULL; i++) {
	    for (j = 0; (fallback = fontFallbacks[i][j]) != NULL; j++) {
		if (strcasecmp(want.fa.family, fallback) == 0) {
		    break;
		}
	    }
	    if (fallback != NULL) {
		for (j = 0; (fallback = fontFallbacks[i][j]) != NULL; j++) {
		    nameList = ListFontOrAlias(display, fallback, &numNames);
		    if (numNames != 0) {
			goto found;
		    }
		}
	    }
	}
	nameList = ListFonts(display, "fixed", &numNames);
	if (numNames == 0) {
	    nameList = ListFonts(display, "*", &numNames);
	}
	if (numNames == 0) {
	    return GetSystemFont(display);
	}
    }

  found:
    bestIdx[0] = -1;
    bestIdx[1] = -1;
    bestScore[0] = (unsigned) -1;
    bestScore[1] = (unsigned) -1;
    for (nameIdx = 0; nameIdx < numNames; nameIdx++) {
	FontAttributes got;
	int scalable;
	unsigned score;

	if (TkFontParseXLFD(nameList[nameIdx], &got.fa, &got.xa) != TCL_OK) {
	    continue;
	}
	IdentifySymbolEncodings(&got);
	scalable = (got.fa.size == 0.0);
	score = RankAttributes(&want, &got);
	if (score < bestScore[scalable]) {
	    bestIdx[scalable] = nameIdx;
	    bestScore[scalable] = score;
	}
	if (score == 0) {
	    break;
	}
    }

    fontStructPtr = GetScreenFont(display, &want, nameList, bestIdx,
	    bestScore);
    XFreeFontNames(nameList);

    if (fontStructPtr == NULL) {
	return GetSystemFont(display);
    }
    return fontStructPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * InitFont --
 *
 *	Helper for TkpGetNativeFont() and TkpGetFontFromAttributes().
 *	Initializes the memory for a new UnixFont that wraps the
 *	platform-specific data.
 *
 *	The caller is responsible for initializing the fields of the TkFont
 *	that are used exclusively by the generic TkFont code, and for
 *	releasing those fields before calling TkpDeleteFont().
 *
 * Results:
 *	Fills the WinFont structure.
 *
 * Side effects:
 *	Memory allocated.
 *
 *---------------------------------------------------------------------------
 */

static void
InitFont(
    Tk_Window tkwin,		/* For screen where font will be used. */
    XFontStruct *fontStructPtr,	/* X information about font. */
    UnixFont *fontPtr)		/* Filled with information constructed from
				 * the above arguments. */
{
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    unsigned long value;
    int minHi, maxHi, minLo, maxLo, fixed, width, limit, i, n;
    FontAttributes fa;
    TkFontAttributes *faPtr;
    TkFontMetrics *fmPtr;
    SubFont *controlPtr, *subFontPtr;
    char *pageMap;
    Display *display;

    /*
     * Get all font attributes and metrics.
     */

    display = Tk_Display(tkwin);
    GetFontAttributes(display, fontStructPtr, &fa);

    minHi = fontStructPtr->min_byte1;
    maxHi = fontStructPtr->max_byte1;
    minLo = fontStructPtr->min_char_or_byte2;
    maxLo = fontStructPtr->max_char_or_byte2;

    fixed = 1;
    if (fontStructPtr->per_char != NULL) {
	width = 0;
	limit = (maxHi - minHi + 1) * (maxLo - minLo + 1);
	for (i = 0; i < limit; i++) {
	    n = fontStructPtr->per_char[i].width;
	    if (n != 0) {
		if (width == 0) {
		    width = n;
		} else if (width != n) {
		    fixed = 0;
		    break;
		}
	    }
	}
    }

    fontPtr->font.fid = fontStructPtr->fid;

    faPtr = &fontPtr->font.fa;
    faPtr->family = fa.fa.family;
    faPtr->size = TkFontGetPoints(tkwin, fa.fa.size);
    faPtr->weight = fa.fa.weight;
    faPtr->slant = fa.fa.slant;
    faPtr->underline = 0;
    faPtr->overstrike = 0;

    fmPtr = &fontPtr->font.fm;
    fmPtr->ascent = fontStructPtr->ascent;
    fmPtr->descent = fontStructPtr->descent;
    fmPtr->maxWidth = fontStructPtr->max_bounds.width;
    fmPtr->fixed = fixed;

    fontPtr->display = display;
    fontPtr->pixelSize = (int)(TkFontGetPixels(tkwin, fa.fa.size) + 0.5);
    fontPtr->xa = fa.xa;

    fontPtr->numSubFonts = 1;
    fontPtr->subFontArray = fontPtr->staticSubFonts;
    InitSubFont(display, fontStructPtr, 1, &fontPtr->subFontArray[0]);

    fontPtr->controlSubFont = fontPtr->subFontArray[0];
    subFontPtr = FindSubFontForChar(fontPtr, '0', NULL);
    controlPtr = &fontPtr->controlSubFont;
    controlPtr->fontStructPtr = subFontPtr->fontStructPtr;
    controlPtr->familyPtr = &tsdPtr->controlFamily;
    controlPtr->fontMap = tsdPtr->controlFamily.fontMap;

    pageMap = fontPtr->subFontArray[0].fontMap[0];
    for (i = 0; i < 256; i++) {
	if ((minHi > 0) || (i < minLo) || (i > maxLo)
		|| !((pageMap[i>>3] >> (i&7)) & 1)) {
	    n = 0;
	} else if (fontStructPtr->per_char == NULL) {
	    n = fontStructPtr->max_bounds.width;
	} else {
	    n = fontStructPtr->per_char[i - minLo].width;
	}
	fontPtr->widths[i] = n;
    }

    if (XGetFontProperty(fontStructPtr, XA_UNDERLINE_POSITION, &value)) {
	fontPtr->underlinePos = value;
    } else {
	/*
	 * If the XA_UNDERLINE_POSITION property does not exist, the X manual
	 * recommends using the following value:
	 */

	fontPtr->underlinePos = fontStructPtr->descent / 2;
    }
    fontPtr->barHeight = 0;
    if (XGetFontProperty(fontStructPtr, XA_UNDERLINE_THICKNESS, &value)) {
	fontPtr->barHeight = value;
    }
    if (fontPtr->barHeight == 0) {
	/*
	 * If the XA_UNDERLINE_THICKNESS property does not exist, the X manual
	 * recommends using the width of the stem on a capital letter. I don't
	 * know of a way to get the stem width of a letter, so guess and use
	 * 1/3 the width of a capital I.
	 */

	fontPtr->barHeight = fontPtr->widths['I'] / 3;
	if (fontPtr->barHeight == 0) {
	    fontPtr->barHeight = 1;
	}
    }
    if (fontPtr->underlinePos + fontPtr->barHeight > fontStructPtr->descent) {
	/*
	 * If this set of cobbled together values would cause the bottom of
	 * the underline bar to stick below the descent of the font, jack the
	 * underline up a bit higher.
	 */

	fontPtr->barHeight = fontStructPtr->descent - fontPtr->underlinePos;
	if (fontPtr->barHeight == 0) {
	    fontPtr->underlinePos--;
	    fontPtr->barHeight = 1;
	}
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * ReleaseFont --
 *
 *	Called to release the unix-specific contents of a TkFont. The caller
 *	is responsible for freeing the memory used by the font itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory is freed.
 *
 *---------------------------------------------------------------------------
 */

static void
ReleaseFont(
    UnixFont *fontPtr)		/* The font to delete. */
{
    int i;

    for (i = 0; i < fontPtr->numSubFonts; i++) {
	ReleaseSubFont(fontPtr->display, &fontPtr->subFontArray[i]);
    }
    if (fontPtr->subFontArray != fontPtr->staticSubFonts) {
	ckfree(fontPtr->subFontArray);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * InitSubFont --
 *
 *	Wrap a screen font and load the FontFamily that represents it. Used to
 *	prepare a SubFont so that characters can be mapped from UTF-8 to the
 *	charset of the font.
 *
 * Results:
 *	The subFontPtr is filled with information about the font.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static void
InitSubFont(
    Display *display,		/* Display in which font will be used. */
    XFontStruct *fontStructPtr,	/* The screen font. */
    int base,			/* Non-zero if this SubFont is being used as
				 * the base font for a font object. */
    SubFont *subFontPtr)	/* Filled with SubFont constructed from above
				 * attributes. */
{
    subFontPtr->fontStructPtr = fontStructPtr;
    subFontPtr->familyPtr = AllocFontFamily(display, fontStructPtr, base);
    subFontPtr->fontMap = subFontPtr->familyPtr->fontMap;
}

/*
 *-------------------------------------------------------------------------
 *
 * ReleaseSubFont --
 *
 *	Called to release the contents of a SubFont. The caller is responsible
 *	for freeing the memory used by the SubFont itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory and resources are freed.
 *
 *---------------------------------------------------------------------------
 */

static void
ReleaseSubFont(
    Display *display,		/* Display which owns screen font. */
    SubFont *subFontPtr)	/* The SubFont to delete. */
{
    XFreeFont(display, subFontPtr->fontStructPtr);
    FreeFontFamily(subFontPtr->familyPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * AllocFontFamily --
 *
 *	Find the FontFamily structure associated with the given font name.
 *	The information should be stored by the caller in a SubFont and used
 *	when determining if that SubFont supports a character.
 *
 *	Cannot use the string name used to construct the font as the key,
 *	because the capitalization may not be canonical. Therefore use the
 *	face name actually retrieved from the font metrics as the key.
 *
 * Results:
 *	A pointer to a FontFamily. The reference count in the FontFamily is
 *	automatically incremented. When the SubFont is released, the reference
 *	count is decremented. When no SubFont is using this FontFamily, it may
 *	be deleted.
 *
 * Side effects:
 *	A new FontFamily structure will be allocated if this font family has
 *	not been seen. TrueType character existence metrics are loaded into
 *	the FontFamily structure.
 *
 *-------------------------------------------------------------------------
 */

static FontFamily *
AllocFontFamily(
    Display *display,		/* Display in which font will be used. */
    XFontStruct *fontStructPtr,	/* Screen font whose FontFamily is to be
				 * returned. */
    int base)			/* Non-zero if this font family is to be used
				 * in the base font of a font object. */
{
    FontFamily *familyPtr;
    FontAttributes fa;
    Tcl_Encoding encoding;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    GetFontAttributes(display, fontStructPtr, &fa);
    encoding = Tcl_GetEncoding(NULL, GetEncodingAlias(fa.xa.charset));

    familyPtr = tsdPtr->fontFamilyList;
    for (; familyPtr != NULL; familyPtr = familyPtr->nextPtr) {
	if ((familyPtr->faceName == fa.fa.family)
		&& (familyPtr->foundry == fa.xa.foundry)
		&& (familyPtr->encoding == encoding)) {
	    if (encoding) {
		Tcl_FreeEncoding(encoding);
	    }
	    familyPtr->refCount++;
	    return familyPtr;
	}
    }

    familyPtr = ckalloc(sizeof(FontFamily));
    memset(familyPtr, 0, sizeof(FontFamily));
    familyPtr->nextPtr = tsdPtr->fontFamilyList;
    tsdPtr->fontFamilyList = familyPtr;

    /*
     * Set key for this FontFamily.
     */

    familyPtr->foundry = fa.xa.foundry;
    familyPtr->faceName = fa.fa.family;
    familyPtr->encoding = encoding;

    /*
     * An initial refCount of 2 means that FontFamily information will persist
     * even when the SubFont that loaded the FontFamily is released. Change it
     * to 1 to cause FontFamilies to be unloaded when not in use.
     */

    familyPtr->refCount = 2;

    /*
     * One byte/character fonts have both min_byte1 and max_byte1 0, and
     * max_char_or_byte2 <= 255. Anything else specifies a two byte/character
     * font.
     */

    familyPtr->isTwoByteFont = !(
	    (fontStructPtr->min_byte1 == 0) &&
	    (fontStructPtr->max_byte1 == 0) &&
	    (fontStructPtr->max_char_or_byte2 < 256));
    return familyPtr;
}

/*
 *-------------------------------------------------------------------------
 *
 * FreeFontFamily --
 *
 *	Called to free an FontFamily when the SubFont is finished using it.
 *	Frees the contents of the FontFamily and the memory used by the
 *	FontFamily itself.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static void
FreeFontFamily(
    FontFamily *familyPtr)	/* The FontFamily to delete. */
{
    FontFamily **familyPtrPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    int i;

    if (familyPtr == NULL) {
	return;
    }
    familyPtr->refCount--;
    if (familyPtr->refCount > 0) {
	return;
    }
    if (familyPtr->encoding) {
	Tcl_FreeEncoding(familyPtr->encoding);
    }
    for (i = 0; i < FONTMAP_PAGES; i++) {
	if (familyPtr->fontMap[i] != NULL) {
	    ckfree(familyPtr->fontMap[i]);
	}
    }

    /*
     * Delete from list.
     */

    for (familyPtrPtr = &tsdPtr->fontFamilyList; ; ) {
	if (*familyPtrPtr == familyPtr) {
	    *familyPtrPtr = familyPtr->nextPtr;
	    break;
	}
	familyPtrPtr = &(*familyPtrPtr)->nextPtr;
    }

    ckfree(familyPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * FindSubFontForChar --
 *
 *	Determine which screen font is necessary to use to display the given
 *	character. If the font object does not have a screen font that can
 *	display the character, another screen font may be loaded into the font
 *	object, following a set of preferred fallback rules.
 *
 * Results:
 *	The return value is the SubFont to use to display the given character.
 *
 * Side effects:
 *	The contents of fontPtr are modified to cache the results of the
 *	lookup and remember any SubFonts that were dynamically loaded. The
 *	table of SubFonts might be extended, and if a non-NULL reference to a
 *	subfont pointer is available, it is updated if it previously pointed
 *	into the old subfont table.
 *
 *-------------------------------------------------------------------------
 */

static SubFont *
FindSubFontForChar(
    UnixFont *fontPtr,		/* The font object with which the character
				 * will be displayed. */
    int ch,			/* The Unicode character to be displayed. */
    SubFont **fixSubFontPtrPtr)	/* Subfont reference to fix up if we
				 * reallocate our subfont table. */
{
    int i, j, k, numNames;
    Tk_Uid faceName;
    const char *fallback;
    const char *const *aliases;
    char **nameList;
    const char *const *anyFallbacks;
    const char *const *const *fontFallbacks;
    SubFont *subFontPtr;
    Tcl_DString ds;

    if (ch < 0 || ch > 0x30000) {
	ch = 0xfffd;
    }

    for (i = 0; i < fontPtr->numSubFonts; i++) {
	if (FontMapLookup(&fontPtr->subFontArray[i], ch)) {
	    return &fontPtr->subFontArray[i];
	}
    }

    if (FontMapLookup(&fontPtr->controlSubFont, ch)) {
	return &fontPtr->controlSubFont;
    }

    /*
     * Keep track of all face names that we check, so we don't check some name
     * multiple times if it can be reached by multiple paths.
     */

    Tcl_DStringInit(&ds);

    /*
     * Are there any other fonts with the same face name as the base font that
     * could display this character, e.g., if the base font is
     * adobe:fixed:iso8859-1, we could might be able to use
     * misc:fixed:iso8859-8 or sony:fixed:jisx0208.1983-0
     */

    faceName = fontPtr->font.fa.family;
    if (SeenName(faceName, &ds) == 0) {
	subFontPtr = CanUseFallback(fontPtr, faceName, ch, fixSubFontPtrPtr);
	if (subFontPtr != NULL) {
	    goto end;
	}
    }

    aliases = TkFontGetAliasList(faceName);

    subFontPtr = NULL;
    fontFallbacks = TkFontGetFallbacks();
    for (i = 0; fontFallbacks[i] != NULL; i++) {
	for (j = 0; (fallback = fontFallbacks[i][j]) != NULL; j++) {
	    if (strcasecmp(fallback, faceName) == 0) {
		/*
		 * If the base font has a fallback...
		 */

		goto tryfallbacks;
	    } else if (aliases != NULL) {
		/*
		 * Or if an alias for the base font has a fallback...
		 */

		for (k = 0; aliases[k] != NULL; k++) {
		    if (strcasecmp(fallback, aliases[k]) == 0) {
			goto tryfallbacks;
		    }
		}
	    }
	}
	continue;

    tryfallbacks:

	/*
	 * ...then see if we can use one of the fallbacks, or an alias for one
	 * of the fallbacks.
	 */

	for (j = 0; (fallback = fontFallbacks[i][j]) != NULL; j++) {
	    subFontPtr = CanUseFallbackWithAliases(fontPtr, fallback, ch, &ds,
		    fixSubFontPtrPtr);
	    if (subFontPtr != NULL) {
		goto end;
	    }
	}
    }

    /*
     * See if we can use something from the global fallback list.
     */

    anyFallbacks = TkFontGetGlobalClass();
    for (i = 0; (fallback = anyFallbacks[i]) != NULL; i++) {
	subFontPtr = CanUseFallbackWithAliases(fontPtr, fallback, ch, &ds,
		fixSubFontPtrPtr);
	if (subFontPtr != NULL) {
	    goto end;
	}
    }

    /*
     * Try all face names available in the whole system until we find one that
     * can be used.
     */

    nameList = ListFonts(fontPtr->display, "*", &numNames);
    for (i = 0; i < numNames; i++) {
	fallback = strchr(nameList[i] + 1, '-') + 1;
	strchr(fallback, '-')[0] = '\0';
	if (SeenName(fallback, &ds) == 0) {
	    subFontPtr = CanUseFallback(fontPtr, fallback, ch,
		    fixSubFontPtrPtr);
	    if (subFontPtr != NULL) {
		XFreeFontNames(nameList);
		goto end;
	    }
	}
    }
    XFreeFontNames(nameList);

  end:
    Tcl_DStringFree(&ds);

    if (subFontPtr == NULL) {
	/*
	 * No font can display this character, so it will be displayed as a
	 * control character expansion.
	 */

	subFontPtr = &fontPtr->controlSubFont;
	FontMapInsert(subFontPtr, ch);
    }
    return subFontPtr;
}

/*
 *-------------------------------------------------------------------------
 *
 * FontMapLookup --
 *
 *	See if the screen font can display the given character.
 *
 * Results:
 *	The return value is 0 if the screen font cannot display the character,
 *	non-zero otherwise.
 *
 * Side effects:
 *	New pages are added to the font mapping cache whenever the character
 *	belongs to a page that hasn't been seen before. When a page is loaded,
 *	information about all the characters on that page is stored, not just
 *	for the single character in question.
 *
 *-------------------------------------------------------------------------
 */

static int
FontMapLookup(
    SubFont *subFontPtr,	/* Contains font mapping cache to be queried
				 * and possibly updated. */
    int ch)			/* Character to be tested. */
{
    int row, bitOffset;

    if (ch < 0 ||  ch >= 0x30000) {
	return 0;
    }
    row = ch >> FONTMAP_SHIFT;
    if (subFontPtr->fontMap[row] == NULL) {
	FontMapLoadPage(subFontPtr, row);
    }
    bitOffset = ch & (FONTMAP_BITSPERPAGE - 1);
    return (subFontPtr->fontMap[row][bitOffset >> 3] >> (bitOffset & 7)) & 1;
}

/*
 *-------------------------------------------------------------------------
 *
 * FontMapInsert --
 *
 *	Tell the font mapping cache that the given screen font should be used
 *	to display the specified character. This is called when no font on the
 *	system can be be found that can display that character; we lie to the
 *	font and tell it that it can display the character, otherwise we would
 *	end up re-searching the entire fallback hierarchy every time that
 *	character was seen.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New pages are added to the font mapping cache whenever the character
 *	belongs to a page that hasn't been seen before. When a page is loaded,
 *	information about all the characters on that page is stored, not just
 *	for the single character in question.
 *
 *-------------------------------------------------------------------------
 */

static void
FontMapInsert(
    SubFont *subFontPtr,	/* Contains font mapping cache to be
				 * updated. */
    int ch)			/* Character to be added to cache. */
{
    int row, bitOffset;

    if (ch >= 0 &&  ch < 0x30000) {
	row = ch >> FONTMAP_SHIFT;
	if (subFontPtr->fontMap[row] == NULL) {
	    FontMapLoadPage(subFontPtr, row);
	}
	bitOffset = ch & (FONTMAP_BITSPERPAGE - 1);
	subFontPtr->fontMap[row][bitOffset >> 3] |= 1 << (bitOffset & 7);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * FontMapLoadPage --
 *
 *	Load information about all the characters on a given page. This
 *	information consists of one bit per character that indicates whether
 *	the associated screen font can (1) or cannot (0) display the
 *	characters on the page.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated.
 *
 *-------------------------------------------------------------------------
 */
static void
FontMapLoadPage(
    SubFont *subFontPtr,	/* Contains font mapping cache to be
				 * updated. */
    int row)			/* Index of the page to be loaded into the
				 * cache. */
{
    char buf[16], src[6];
    int minHi, maxHi, minLo, maxLo, scale, checkLo;
    int i, end, bitOffset, isTwoByteFont, n;
    Tcl_Encoding encoding;
    XFontStruct *fontStructPtr;
    XCharStruct *widths;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    subFontPtr->fontMap[row] = ckalloc(FONTMAP_BITSPERPAGE / 8);
    memset(subFontPtr->fontMap[row], 0, FONTMAP_BITSPERPAGE / 8);

    if (subFontPtr->familyPtr == &tsdPtr->controlFamily) {
	return;
    }

    fontStructPtr = subFontPtr->fontStructPtr;
    encoding = subFontPtr->familyPtr->encoding;
    isTwoByteFont = subFontPtr->familyPtr->isTwoByteFont;

    widths = fontStructPtr->per_char;
    minHi = fontStructPtr->min_byte1;
    maxHi = fontStructPtr->max_byte1;
    minLo = fontStructPtr->min_char_or_byte2;
    maxLo = fontStructPtr->max_char_or_byte2;
    scale = maxLo - minLo + 1;
    checkLo = minLo;

    if (! isTwoByteFont) {
	if (minLo < 32) {
	    checkLo = 32;
	}
    }

    end = (row + 1) << FONTMAP_SHIFT;
    for (i = row << FONTMAP_SHIFT; i < end; i++) {
	int hi, lo;

	if (Tcl_UtfToExternal(NULL, encoding, src, TkUniCharToUtf(i, src),
		TCL_ENCODING_STOPONERROR, NULL, buf, sizeof(buf), NULL,
		NULL, NULL) != TCL_OK) {
	    continue;
	}
	if (isTwoByteFont) {
	    hi = ((unsigned char *) buf)[0];
	    lo = ((unsigned char *) buf)[1];
	} else {
	    hi = 0;
	    lo = ((unsigned char *) buf)[0];
	}
	if ((hi < minHi) || (hi > maxHi) || (lo < checkLo) || (lo > maxLo)) {
	    continue;
	}
	n = (hi - minHi) * scale + lo - minLo;
	if ((widths == NULL) || (widths[n].width + widths[n].rbearing != 0)) {
	    bitOffset = i & (FONTMAP_BITSPERPAGE - 1);
	    subFontPtr->fontMap[row][bitOffset >> 3] |= 1 << (bitOffset & 7);
	}
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * CanUseFallbackWithAliases --
 *
 *	Helper function for FindSubFontForChar. Determine if the specified
 *	face name (or an alias of the specified face name) can be used to
 *	construct a screen font that can display the given character.
 *
 * Results:
 *	See CanUseFallback().
 *
 * Side effects:
 *	If the name and/or one of its aliases was rejected, the rejected
 *	string is recorded in nameTriedPtr so that it won't be tried again.
 *	The table of SubFonts might be extended, and if a non-NULL reference
 *	to a subfont pointer is available, it is updated if it previously
 *	pointed into the old subfont table.
 *
 *---------------------------------------------------------------------------
 */

static SubFont *
CanUseFallbackWithAliases(
    UnixFont *fontPtr,		/* The font object that will own the new
				 * screen font. */
    const char *faceName,		/* Desired face name for new screen font. */
    int ch,			/* The Unicode character that the new screen
				 * font must be able to display. */
    Tcl_DString *nameTriedPtr,	/* Records face names that have already been
				 * tried. It is possible for the same face
				 * name to be queried multiple times when
				 * trying to find a suitable screen font. */
    SubFont **fixSubFontPtrPtr)	/* Subfont reference to fix up if we
				 * reallocate our subfont table. */
{
    SubFont *subFontPtr;
    const char *const *aliases;
    int i;

    if (SeenName(faceName, nameTriedPtr) == 0) {
	subFontPtr = CanUseFallback(fontPtr, faceName, ch, fixSubFontPtrPtr);
	if (subFontPtr != NULL) {
	    return subFontPtr;
	}
    }
    aliases = TkFontGetAliasList(faceName);
    if (aliases != NULL) {
	for (i = 0; aliases[i] != NULL; i++) {
	    if (SeenName(aliases[i], nameTriedPtr) == 0) {
		subFontPtr = CanUseFallback(fontPtr, aliases[i], ch,
			fixSubFontPtrPtr);
		if (subFontPtr != NULL) {
		    return subFontPtr;
		}
	    }
	}
    }
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * SeenName --
 *
 *	Used to determine we have already tried and rejected the given face
 *	name when looking for a screen font that can support some Unicode
 *	character.
 *
 * Results:
 *	The return value is 0 if this face name has not already been seen,
 *	non-zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
SeenName(
    const char *name,		/* The name to check. */
    Tcl_DString *dsPtr)		/* Contains names that have already been
				 * seen. */
{
    const char *seen, *end;

    seen = Tcl_DStringValue(dsPtr);
    end = seen + Tcl_DStringLength(dsPtr);
    while (seen < end) {
	if (strcasecmp(seen, name) == 0) {
	    return 1;
	}
	seen += strlen(seen) + 1;
    }
    Tcl_DStringAppend(dsPtr, name, (int) (strlen(name) + 1));
    return 0;
}

/*
 *-------------------------------------------------------------------------
 *
 * CanUseFallback --
 *
 *	If the specified screen font has not already been loaded into the font
 *	object, determine if the specified screen font can display the given
 *	character.
 *
 * Results:
 *	The return value is a pointer to a newly allocated SubFont, owned by
 *	the font object. This SubFont can be used to display the given
 *	character. The SubFont represents the screen font with the base set of
 *	font attributes from the font object, but using the specified face
 *	name. NULL is returned if the font object already holds a reference to
 *	the specified font or if the specified font doesn't exist or cannot
 *	display the given character.
 *
 * Side effects:
 *	The font object's subFontArray is updated to contain a reference to
 *	the newly allocated SubFont. The table of SubFonts might be extended,
 *	and if a non-NULL reference to a subfont pointer is available, it is
 *	updated if it previously pointed into the old subfont table.
 *
 *-------------------------------------------------------------------------
 */

static SubFont *
CanUseFallback(
    UnixFont *fontPtr,		/* The font object that will own the new
				 * screen font. */
    const char *faceName,	/* Desired face name for new screen font. */
    int ch,			/* The Unicode character that the new screen
				 * font must be able to display. */
    SubFont **fixSubFontPtrPtr)	/* Subfont reference to fix up if we
				 * reallocate our subfont table. */
{
    int i, nameIdx, numNames, srcLen, numEncodings, bestIdx[2];
    Tk_Uid hateFoundry;
    const char *charset, *hateCharset;
    unsigned bestScore[2];
    char **nameList;
    char **nameListOrig;
    char src[6];
    FontAttributes want, got;
    Display *display;
    SubFont subFont;
    XFontStruct *fontStructPtr;
    Tcl_DString dsEncodings;
    Tcl_Encoding *encodingCachePtr;

    /*
     * Assume: the face name is times.
     * Assume: adobe:times:iso8859-1 has already been used.
     *
     * Are there any versions of times that can display this character (e.g.,
     *    perhaps linotype:times:iso8859-2)?
     *	  a. Get list of all times fonts.
     *	  b1. Cross out all names whose encodings we've already used.
     *	  b2. Cross out all names whose foundry & encoding we've already seen.
     *	  c. Cross out all names whose encoding cannot handle the character.
     *	  d. Rank each name and pick the best match.
     *	  e. If that font cannot actually display the character, cross out all
     *	     names with the same foundry and encoding and go back to (c).
     */

    display = fontPtr->display;
    nameList = ListFonts(display, faceName, &numNames);
    if (numNames == 0) {
	return NULL;
    }
    nameListOrig = nameList;

    srcLen = TkUniCharToUtf(ch, src);

    want.fa = fontPtr->font.fa;
    want.xa = fontPtr->xa;

    want.fa.family = Tk_GetUid(faceName);
    want.fa.size = (double)-fontPtr->pixelSize;

    hateFoundry = NULL;
    hateCharset = NULL;
    numEncodings = 0;
    Tcl_DStringInit(&dsEncodings);

    charset = NULL;	/* lint, since numNames must be > 0 to get here. */

  retry:
    bestIdx[0] = -1;
    bestIdx[1] = -1;
    bestScore[0] = (unsigned) -1;
    bestScore[1] = (unsigned) -1;
    for (nameIdx = 0; nameIdx < numNames; nameIdx++) {
	Tcl_Encoding encoding;
	char dst[16];
	int scalable, srcRead, dstWrote;
	unsigned score;

	if (nameList[nameIdx] == NULL) {
	    continue;
	}
	if (TkFontParseXLFD(nameList[nameIdx], &got.fa, &got.xa) != TCL_OK) {
	    goto crossout;
	}
	IdentifySymbolEncodings(&got);
	charset = GetEncodingAlias(got.xa.charset);
	if (hateFoundry != NULL) {
	    /*
	     * E. If the font we picked cannot actually display the character,
	     * cross out all names with the same foundry and encoding.
	     */

	    if ((hateFoundry == got.xa.foundry)
		    && (strcmp(hateCharset, charset) == 0)) {
		goto crossout;
	    }
	} else {
	    /*
	     * B. Cross out all names whose encodings we've already used.
	     */

	    for (i = 0; i < fontPtr->numSubFonts; i++) {
		encoding = fontPtr->subFontArray[i].familyPtr->encoding;
		if (strcmp(charset, Tcl_GetEncodingName(encoding)) == 0) {
		    goto crossout;
		}
	    }
	}

	/*
	 * C. Cross out all names whose encoding cannot handle the character.
	 */

	encodingCachePtr = (Tcl_Encoding *) Tcl_DStringValue(&dsEncodings);
	for (i = numEncodings; --i >= 0; encodingCachePtr++) {
	    encoding = *encodingCachePtr;
	    if (strcmp(Tcl_GetEncodingName(encoding), charset) == 0) {
		break;
	    }
	}
	if (i < 0) {
	    encoding = Tcl_GetEncoding(NULL, charset);
	    if (encoding == NULL) {
		goto crossout;
	    }

	    Tcl_DStringAppend(&dsEncodings, (char *) &encoding,
		    sizeof(encoding));
	    numEncodings++;
	}
	Tcl_UtfToExternal(NULL, encoding, src, srcLen,
		TCL_ENCODING_STOPONERROR, NULL, dst, sizeof(dst), &srcRead,
		&dstWrote, NULL);
	if (dstWrote == 0) {
	    goto crossout;
	}

	/*
	 * D. Rank each name and pick the best match.
	 */

	scalable = (got.fa.size == 0.0);
	score = RankAttributes(&want, &got);
	if (score < bestScore[scalable]) {
	    bestIdx[scalable] = nameIdx;
	    bestScore[scalable] = score;
	}
	if (score == 0) {
	    break;
	}
	continue;

    crossout:
	if (nameList == nameListOrig) {
	    /*
	     * Not allowed to change pointers to memory that X gives you, so
	     * make a copy.
	     */

	    nameList = ckalloc(numNames * sizeof(char *));
	    memcpy(nameList, nameListOrig, numNames * sizeof(char *));
	}
	nameList[nameIdx] = NULL;
    }

    fontStructPtr = GetScreenFont(display, &want, nameList, bestIdx,
	    bestScore);

    encodingCachePtr = (Tcl_Encoding *) Tcl_DStringValue(&dsEncodings);
    for (i = numEncodings; --i >= 0; encodingCachePtr++) {
	Tcl_FreeEncoding(*encodingCachePtr);
    }
    Tcl_DStringFree(&dsEncodings);
    numEncodings = 0;

    if (fontStructPtr == NULL) {
	if (nameList != nameListOrig) {
	    ckfree(nameList);
	}
	XFreeFontNames(nameListOrig);
	return NULL;
    }

    InitSubFont(display, fontStructPtr, 0, &subFont);
    if (FontMapLookup(&subFont, ch) == 0) {
	/*
	 * E. If the font we picked cannot actually display the character,
	 * cross out all names with the same foundry and encoding and pick
	 * another font.
	 */

	hateFoundry = got.xa.foundry;
	hateCharset = charset;
	ReleaseSubFont(display, &subFont);
	goto retry;
    }
    if (nameList != nameListOrig) {
	ckfree(nameList);
    }
    XFreeFontNames(nameListOrig);

    if (fontPtr->numSubFonts >= SUBFONT_SPACE) {
	SubFont *newPtr;

	newPtr = ckalloc(sizeof(SubFont) * (fontPtr->numSubFonts + 1));
	memcpy(newPtr, fontPtr->subFontArray,
		fontPtr->numSubFonts * sizeof(SubFont));
	if (fixSubFontPtrPtr != NULL) {
	    register SubFont *fixSubFontPtr = *fixSubFontPtrPtr;

	    if (fixSubFontPtr != &fontPtr->controlSubFont) {
		*fixSubFontPtrPtr =
			newPtr + (fixSubFontPtr - fontPtr->subFontArray);
	    }
	}
	if (fontPtr->subFontArray != fontPtr->staticSubFonts) {
	    ckfree(fontPtr->subFontArray);
	}
	fontPtr->subFontArray = newPtr;
    }
    fontPtr->subFontArray[fontPtr->numSubFonts] = subFont;
    fontPtr->numSubFonts++;
    return &fontPtr->subFontArray[fontPtr->numSubFonts - 1];
}

/*
 *---------------------------------------------------------------------------
 *
 * RankAttributes --
 *
 *	Determine how close the attributes of the font in question match the
 *	attributes that we want.
 *
 * Results:
 *	The return value is the score; lower numbers are better. *scalablePtr
 *	is set to 0 if the font was not scalable, 1 otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static unsigned
RankAttributes(
    FontAttributes *wantPtr,	/* The desired attributes. */
    FontAttributes *gotPtr)	/* The attributes we have to live with. */
{
    unsigned penalty;

    penalty = 0;
    if (gotPtr->xa.foundry != wantPtr->xa.foundry) {
	penalty += 4500;
    }
    if (gotPtr->fa.family != wantPtr->fa.family) {
	penalty += 9000;
    }
    if (gotPtr->fa.weight != wantPtr->fa.weight) {
	penalty += 90;
    }
    if (gotPtr->fa.slant != wantPtr->fa.slant) {
	penalty += 60;
    }
    if (gotPtr->xa.slant != wantPtr->xa.slant) {
	penalty += 10;
    }
    if (gotPtr->xa.setwidth != wantPtr->xa.setwidth) {
	penalty += 1000;
    }

    if (gotPtr->fa.size == 0.0) {
	/*
	 * A scalable font is almost always acceptable, but the corresponding
	 * bitmapped font would be better.
	 */

	penalty += 10;
    } else {
	int diff;

	/*
	 * It's worse to be too large than to be too small.
	 */

	diff = (int) (150 * (-gotPtr->fa.size - -wantPtr->fa.size));
	if (diff > 0) {
	    penalty += 600;
	} else if (diff < 0) {
	    penalty += 150;
	    diff = -diff;
	}
	penalty += diff;
    }
    if (gotPtr->xa.charset != wantPtr->xa.charset) {
	int i;
	const char *gotAlias, *wantAlias;

	penalty += 65000;
	gotAlias = GetEncodingAlias(gotPtr->xa.charset);
	wantAlias = GetEncodingAlias(wantPtr->xa.charset);
	if (strcmp(gotAlias, wantAlias) != 0) {
	    penalty += 30000;
	    for (i = 0; encodingList[i] != NULL; i++) {
		if (strcmp(gotAlias, encodingList[i]) == 0) {
		    penalty -= 30000;
		    break;
		}
		penalty += 20000;
	    }
	}
    }
    return penalty;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetScreenFont --
 *
 *	Given the names for the best scalable and best bitmapped font,
 *	actually construct an XFontStruct based on the best XLFD. This is
 *	where all the alias and fallback substitution bottoms out.
 *
 * Results:
 *	The screen font that best corresponds to the set of attributes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static XFontStruct *
GetScreenFont(
    Display *display,		/* Display for new XFontStruct. */
    FontAttributes *wantPtr,	/* Contains desired actual pixel-size if the
				 * best font was scalable. */
    char **nameList,		/* Array of XLFDs. */
    int bestIdx[2],		/* Indices into above array for XLFD of best
				 * bitmapped and best scalable font. */
    unsigned bestScore[2])	/* Scores of best bitmapped and best scalable
				 * font. XLFD corresponding to lowest score
				 * will be constructed. */
{
    XFontStruct *fontStructPtr;

    if ((bestIdx[0] < 0) && (bestIdx[1] < 0)) {
	return NULL;
    }

    /*
     * Now we know which is the closest matching scalable font and the closest
     * matching bitmapped font. If the scalable font was a better match, try
     * getting the scalable font; however, if the scalable font was not
     * actually available in the desired pointsize, fall back to the closest
     * bitmapped font.
     */

    fontStructPtr = NULL;
    if (bestScore[1] < bestScore[0]) {
	char *str, *rest, buf[256];
	int i;

	/*
	 * Fill in the desired pixel size for this font.
	 */

    tryscale:
	str = nameList[bestIdx[1]];
	for (i = 0; i < XLFD_PIXEL_SIZE; i++) {
	    str = strchr(str + 1, '-');
	}
	rest = str;
	for (i = XLFD_PIXEL_SIZE; i < XLFD_CHARSET; i++) {
	    rest = strchr(rest + 1, '-');
	}
	*str = '\0';
	sprintf(buf, "%.200s-%d-*-*-*-*-*%s", nameList[bestIdx[1]],
		(int)(-wantPtr->fa.size+0.5), rest);
	*str = '-';
	fontStructPtr = XLoadQueryFont(display, buf);
	bestScore[1] = INT_MAX;
    }
    if (fontStructPtr == NULL) {
	fontStructPtr = XLoadQueryFont(display, nameList[bestIdx[0]]);
	if (fontStructPtr == NULL) {
	    /*
	     * This shouldn't happen because the font name is one of the names
	     * that X gave us to use, but it does anyhow.
	     */

	    if (bestScore[1] < INT_MAX) {
		goto tryscale;
	    }
	    return GetSystemFont(display);
	}
    }
    return fontStructPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetSystemFont --
 *
 *	Absolute fallback mechanism, called when we need a font and no other
 *	font can be found and/or instantiated.
 *
 * Results:
 *	A pointer to a font. Never NULL.
 *
 * Side effects:
 *	If there are NO fonts installed on the system, this call will panic,
 *	but how did you get X running in that case?
 *
 *---------------------------------------------------------------------------
 */

static XFontStruct *
GetSystemFont(
    Display *display)		/* Display for new XFontStruct. */
{
    XFontStruct *fontStructPtr;

    fontStructPtr = XLoadQueryFont(display, "fixed");
    if (fontStructPtr == NULL) {
	fontStructPtr = XLoadQueryFont(display, "*");
	if (fontStructPtr == NULL) {
	    Tcl_Panic("TkpGetFontFromAttributes: cannot get any font");
	}
    }
    return fontStructPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetFontAttributes --
 *
 *	Given a screen font, determine its actual attributes, which are not
 *	necessarily the attributes that were used to construct it.
 *
 * Results:
 *	*faPtr is filled with the screen font's attributes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
GetFontAttributes(
    Display *display,		/* Display that owns the screen font. */
    XFontStruct *fontStructPtr,	/* Screen font to query. */
    FontAttributes *faPtr)	/* For storing attributes of screen font. */
{
    unsigned long value;
    char *name;

    if ((XGetFontProperty(fontStructPtr, XA_FONT, &value) != False) &&
	    (value != 0)) {
	name = XGetAtomName(display, (Atom) value);
	if (TkFontParseXLFD(name, &faPtr->fa, &faPtr->xa) != TCL_OK) {
	    faPtr->fa.family = Tk_GetUid(name);
	    faPtr->xa.foundry = Tk_GetUid("");
	    faPtr->xa.charset = Tk_GetUid("");
	}
	XFree(name);
    } else {
	TkInitFontAttributes(&faPtr->fa);
	TkInitXLFDAttributes(&faPtr->xa);
    }

    /*
     * Do last ditch check for family. It seems that some X servers can fail
     * on the X font calls above, slipping through earlier checks. X-Win32 5.4
     * is one of these.
     */

    if (faPtr->fa.family == NULL) {
	faPtr->fa.family = Tk_GetUid("");
	faPtr->xa.foundry = Tk_GetUid("");
	faPtr->xa.charset = Tk_GetUid("");
    }
    return IdentifySymbolEncodings(faPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * ListFonts --
 *
 *	Utility function to return the array of all XLFDs on the system with
 *	the specified face name.
 *
 * Results:
 *	The return value is an array of XLFDs, which should be freed with
 *	XFreeFontNames(), or NULL if no XLFDs matched the requested name.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static char **
ListFonts(
    Display *display,		/* Display to query. */
    const char *faceName,	/* Desired face name, or "*" for all. */
    int *numNamesPtr)		/* Filled with length of returned array, or 0
				 * if no names were found. */
{
    char buf[256];

    sprintf(buf, "-*-%.80s-*-*-*-*-*-*-*-*-*-*-*-*", faceName);
    return XListFonts(display, buf, 10000, numNamesPtr);
}

static char **
ListFontOrAlias(
    Display *display,		/* Display to query. */
    const char *faceName,	/* Desired face name, or "*" for all. */
    int *numNamesPtr)		/* Filled with length of returned array, or 0
				 * if no names were found. */
{
    char **nameList;
    const char *const *aliases;
    int i;

    nameList = ListFonts(display, faceName, numNamesPtr);
    if (nameList != NULL) {
	return nameList;
    }
    aliases = TkFontGetAliasList(faceName);
    if (aliases != NULL) {
	for (i = 0; aliases[i] != NULL; i++) {
	    nameList = ListFonts(display, aliases[i], numNamesPtr);
	    if (nameList != NULL) {
		return nameList;
	    }
	}
    }
    *numNamesPtr = 0;
    return NULL;
}

/*
 *---------------------------------------------------------------------------
 *
 * IdentifySymbolEncodings --
 *
 *	If the font attributes refer to a symbol font, update the charset
 *	field of the font attributes so that it reflects the encoding of that
 *	symbol font. In general, the raw value for the charset field parsed
 *	from an XLFD is meaningless for symbol fonts.
 *
 *	Symbol fonts are all fonts whose name appears in the symbolClass.
 *
 * Results:
 *	The return value is non-zero if the font attributes specify a symbol
 *	font, or 0 otherwise. If a non-zero value is returned the charset
 *	field of the font attributes will be changed to the string that
 *	represents the actual encoding for the symbol font.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
IdentifySymbolEncodings(
    FontAttributes *faPtr)
{
    int i, j;
    const char *const *aliases;
    const char *const *symbolClass;

    symbolClass = TkFontGetSymbolClass();
    for (i = 0; symbolClass[i] != NULL; i++) {
	if (strcasecmp(faPtr->fa.family, symbolClass[i]) == 0) {
	    faPtr->xa.charset = Tk_GetUid(GetEncodingAlias(symbolClass[i]));
	    return 1;
	}
	aliases = TkFontGetAliasList(symbolClass[i]);
	for (j = 0; (aliases != NULL) && (aliases[j] != NULL); j++) {
	    if (strcasecmp(faPtr->fa.family, aliases[j]) == 0) {
		faPtr->xa.charset = Tk_GetUid(GetEncodingAlias(aliases[j]));
		return 1;
	    }
	}
    }
    return 0;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetEncodingAlias --
 *
 *	Map the name of an encoding to another name that should be used when
 *	actually loading the encoding. For instance, the encodings
 *	"jisc6226.1978", "jisx0208.1983", "jisx0208.1990", and "jisx0208.1996"
 *	are well-known names for the same encoding and are represented by one
 *	encoding table: "jis0208".
 *
 * Results:
 *	As above. If the name has no alias, the original name is returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static const char *
GetEncodingAlias(
    const char *name)		/* The name to look up. */
{
    const EncodingAlias *aliasPtr;

    for (aliasPtr = encodingAliases; aliasPtr->aliasPattern != NULL; ) {
	if (Tcl_StringCaseMatch(name, aliasPtr->aliasPattern, 0)) {
	    return aliasPtr->realName;
	}
	aliasPtr++;
    }
    return name;
}

/*
 *---------------------------------------------------------------------------
 *
 * TkDrawAngledChars --
 *
 *	Draw some characters at an angle. This is awkward here because we have
 *	no reliable way of drawing any characters at an angle in classic X11;
 *	we have to draw on a Pixmap which is converted to an XImage (from
 *	helper function GetImageOfText), rotate the image (hokey code!) onto
 *	another XImage (from helper function InitDestImage), and then use the
 *	rotated image as a mask when drawing. This is pretty awful; improved
 *	versions are welcomed!
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Target drawable is updated.
 *
 *---------------------------------------------------------------------------
 */

static inline XImage *
GetImageOfText(
    Display *display,		/* Display on which to draw. */
    Drawable drawable,		/* Window or pixmap in which to draw. */
    Tk_Font tkfont,		/* Font in which characters will be drawn. */
    const char *source,		/* UTF-8 string to be displayed. Need not be
				 * '\0' terminated. All Tk meta-characters
				 * (tabs, control characters, and newlines)
				 * should be stripped out of the string that
				 * is passed to this function. If they are not
				 * stripped out, they will be displayed as
				 * regular printing characters. */
    int numBytes,		/* Number of bytes in string. */
    int *realWidthPtr, int *realHeightPtr)
{
    int width, height;
    TkFont *fontPtr = (TkFont *) tkfont;
    Pixmap bitmap;
    GC bitmapGC;
    XGCValues values;
    XImage *image;

    (void) Tk_MeasureChars(tkfont, source, numBytes, -1, 0, &width);
    height = fontPtr->fm.ascent + fontPtr->fm.descent;

    bitmap = Tk_GetPixmap(display, drawable, width, height, 1);
    values.graphics_exposures = False;
    values.foreground = BlackPixel(display, DefaultScreen(display));
    bitmapGC = XCreateGC(display, bitmap, GCGraphicsExposures|GCForeground,
	    &values);
    XFillRectangle(display, bitmap, bitmapGC, 0, 0, width, height);

    values.font = Tk_FontId(tkfont);
    values.foreground = WhitePixel(display, DefaultScreen(display));
    values.background = BlackPixel(display, DefaultScreen(display));
    XChangeGC(display, bitmapGC, GCFont|GCForeground|GCBackground, &values);
    Tk_DrawChars(display, bitmap, bitmapGC, tkfont, source, numBytes, 0,
	    fontPtr->fm.ascent);
    XFreeGC(display, bitmapGC);

    image = XGetImage(display, bitmap, 0, 0, width, height, AllPlanes,
	    ZPixmap);
    Tk_FreePixmap(display, bitmap);

    *realWidthPtr = width;
    *realHeightPtr = height;
    return image;
}

static inline XImage *
InitDestImage(
    Display *display,
    Drawable drawable,
    int width,
    int height,
    Pixmap *bitmapPtr)
{
    Pixmap bitmap;
    XImage *image;
    GC bitmapGC;
    XGCValues values;

    bitmap = Tk_GetPixmap(display, drawable, width, height, 1);
    values.graphics_exposures = False;
    values.foreground = BlackPixel(display, DefaultScreen(display));
    bitmapGC = XCreateGC(display, bitmap, GCGraphicsExposures|GCForeground,
	    &values);
    XFillRectangle(display, bitmap, bitmapGC, 0, 0, width, height);
    XFreeGC(display, bitmapGC);

    image = XGetImage(display, bitmap, 0, 0, width, height, AllPlanes,
	    ZPixmap);
    *bitmapPtr = bitmap;
    return image;
}

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
    double x, double y,
    double angle)
{
    if (angle == 0.0) {
	Tk_DrawChars(display, drawable, gc, tkfont, source, numBytes, x, y);
    } else {
	double sinA = sin(angle * PI/180.0), cosA = cos(angle * PI/180.0);
	int bufHeight, bufWidth, srcWidth, srcHeight, i, j, dx, dy;
	Pixmap buf;
	XImage *srcImage = GetImageOfText(display, drawable, tkfont, source,
		numBytes, &srcWidth, &srcHeight);
	XImage *dstImage;
	enum {Q0=1,R1,Q1,R2,Q2,R3,Q3} quadrant;
	GC bwgc, cpgc;
	XGCValues values;
	int ascent = ((TkFont *) tkfont)->fm.ascent;

	/*
	 * First, work out what quadrant we are operating in. We also handle
	 * the rectilinear rotations as special cases. Conceptually, there's
	 * also R0 (angle == 0.0) but that has been already handled as a
	 * special case above.
	 *
	 *        R1
	 *   Q1   |   Q0
	 *        |
	 * R2 ----+---- R0
	 *        |
	 *   Q2   |   Q3
	 *        R3
	 */

	if (angle < 90.0) {
	    quadrant = Q0;
	} else if (angle == 90.0) {
	    quadrant = R1;
	} else if (angle < 180.0) {
	    quadrant = Q1;
	} else if (angle == 180.0) {
	    quadrant = R2;
	} else if (angle < 270.0) {
	    quadrant = Q2;
	} else if (angle == 270.0) {
	    quadrant = R3;
	} else {
	    quadrant = Q3;
	}

	if (srcImage == NULL) {
	    return;
	}
	bufWidth = srcWidth*fabs(cosA) + srcHeight*fabs(sinA);
	bufHeight = srcHeight*fabs(cosA) + srcWidth*fabs(sinA);
	dstImage = InitDestImage(display, drawable, bufWidth,bufHeight, &buf);
	if (dstImage == NULL) {
	    Tk_FreePixmap(display, buf);
	    XDestroyImage(srcImage);
	    return;
	}

	/*
	 * Do the rotation, setting or resetting pixels in the destination
	 * image dependent on whether the corresponding pixel (after rotation
	 * to source image space) is set.
	 */

	for (i=0 ; i<srcWidth ; i++) {
	    for (j=0 ; j<srcHeight ; j++) {
		switch (quadrant) {
		case Q0:
		    dx = ROUND16(i*cosA + j*sinA);
		    dy = ROUND16(j*cosA + (srcWidth - i)*sinA);
		    break;
		case R1:
		    dx = j;
		    dy = srcWidth - i;
		    break;
		case Q1:
		    dx = ROUND16((i - srcWidth)*cosA + j*sinA);
		    dy = ROUND16((srcWidth-i)*sinA + (j-srcHeight)*cosA);
		    break;
		case R2:
		    dx = srcWidth - i;
		    dy = srcHeight - j;
		    break;
		case Q2:
		    dx = ROUND16((i-srcWidth)*cosA + (j-srcHeight)*sinA);
		    dy = ROUND16((j - srcHeight)*cosA - i*sinA);
		    break;
		case R3:
		    dx = srcHeight - j;
		    dy = i;
		    break;
		default:
		    dx = ROUND16(i*cosA + (j - srcHeight)*sinA);
		    dy = ROUND16(j*cosA - i*sinA);
		}

		if (dx < 0 || dy < 0 || dx >= bufWidth || dy >= bufHeight) {
		    continue;
		}
		XPutPixel(dstImage, dx, dy,
			XGetPixel(dstImage,dx,dy) | XGetPixel(srcImage,i,j));
	    }
	}
	XDestroyImage(srcImage);

	/*
	 * Schlep the data back to the Xserver.
	 */

	values.function = GXcopy;
	values.foreground = WhitePixel(display, DefaultScreen(display));
	values.background = BlackPixel(display, DefaultScreen(display));
	bwgc = XCreateGC(display, buf, GCFunction|GCForeground|GCBackground,
		&values);
	XPutImage(display, buf, bwgc, dstImage, 0,0, 0,0, bufWidth,bufHeight);
	XFreeGC(display, bwgc);
	XDestroyImage(dstImage);

	/*
	 * Calculate where we want to draw the text.
	 */

	switch (quadrant) {
	case Q0:
	    dx = x;
	    dy = y - srcWidth*sinA;
	    break;
	case R1:
	    dx = x;
	    dy = y - srcWidth;
	    break;
	case Q1:
	    dx = x + srcWidth*cosA;
	    dy = y + srcHeight*cosA - srcWidth*sinA;
	    break;
	case R2:
	    dx = x - srcWidth;
	    dy = y - srcHeight;
	    break;
	case Q2:
	    dx = x + srcWidth*cosA + srcHeight*sinA;
	    dy = y + srcHeight*cosA;
	    break;
	case R3:
	    dx = x - srcHeight;
	    dy = y;
	    break;
	default:
	    dx = x + srcHeight*sinA;
	    dy = y;
	}

	/*
	 * Apply a correction to deal with the fact that we aren't told to
	 * draw from our top-left corner but rather from the left-end of our
	 * baseline.
	 */

	dx -= ascent*sinA;
	dy -= ascent*cosA;

	/*
	 * Transfer the text to the screen. This is done by using it as a mask
	 * and then drawing through that mask with the original drawing color.
	 */

	values.function = GXcopy;
	values.fill_style = FillSolid;
	values.clip_mask = buf;
	values.clip_x_origin = dx;
	values.clip_y_origin = dy;
	cpgc = XCreateGC(display, drawable,
		GCFunction|GCFillStyle|GCClipMask|GCClipXOrigin|GCClipYOrigin,
		&values);
	XCopyGC(display, gc, GCForeground, cpgc);
	XFillRectangle(display, drawable, cpgc, dx, dy, bufWidth,
		bufHeight);
	XFreeGC(display, cpgc);

	Tk_FreePixmap(display, buf);
	return;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
