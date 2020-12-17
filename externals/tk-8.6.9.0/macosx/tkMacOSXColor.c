/*
 * tkMacOSXColor.c --
 *
 *	This file maintains a database of color values for the Tk
 *	toolkit, in order to avoid round-trips to the server to
 *	map color names to pixel values.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkColor.h"

struct SystemColorMapEntry {
    const char *name;
    ThemeBrush brush;
    ThemeTextColor textColor;
    ThemeBackgroundKind background;
};  /* unsigned char pixelCode; */

/*
 * Array of system color definitions: the array index is required to equal the
 * color's (pixelCode - MIN_PIXELCODE), i.e. the array order needs to be kept
 * in sync with the public pixel code values in tkMacOSXPort.h !
 */

#define MIN_PIXELCODE  30
static const struct SystemColorMapEntry systemColorMap[] = {
    { "Transparent",			    0, 0, 0 },							/*  30: TRANSPARENT_PIXEL */
    { "Highlight",			    kThemeBrushPrimaryHighlightColor, 0, 0 },			/*  31: HIGHLIGHT_PIXEL */
    { "HighlightSecondary",		    kThemeBrushSecondaryHighlightColor, 0, 0 },			/*  32: HIGHLIGHT_SECONDARY_PIXEL */
    { "HighlightText",			    kThemeBrushBlack, 0, 0 },					/*  33: HIGHLIGHT_TEXT_PIXEL */
    { "HighlightAlternate",		    kThemeBrushAlternatePrimaryHighlightColor, 0, 0 },		/*  34: HIGHLIGHT_ALTERNATE_PIXEL */
    { "ButtonText",			    0, kThemeTextColorPushButtonActive, 0 },			/*  35: CONTROL_TEXT_PIXEL */
    { "PrimaryHighlightColor",		    kThemeBrushPrimaryHighlightColor, 0, 0 },			/*  36 */
    { "ButtonFace",			    kThemeBrushButtonFaceActive, 0, 0 },			/*  37: CONTROL_BODY_PIXEL */
    { "SecondaryHighlightColor",	    kThemeBrushSecondaryHighlightColor, 0, 0 },			/*  38 */
    { "ButtonFrame",			    kThemeBrushButtonFrameActive, 0, 0 },			/*  39: CONTROL_FRAME_PIXEL */
    { "AlternatePrimaryHighlightColor",	    kThemeBrushAlternatePrimaryHighlightColor, 0, 0 },		/*  40 */
    { "WindowBody",			    kThemeBrushDocumentWindowBackground, 0, 0 },		/*  41: WINDOW_BODY_PIXEL */
    { "SheetBackground",		    kThemeBrushSheetBackground, 0, 0 },				/*  42 */
    { "MenuActive",			    kThemeBrushMenuBackgroundSelected, 0, 0 },			/*  43: MENU_ACTIVE_PIXEL */
    { "Black",				    kThemeBrushBlack, 0, 0 },					/*  44 */
    { "MenuActiveText",			    0, kThemeTextColorMenuItemSelected, 0 },			/*  45: MENU_ACTIVE_TEXT_PIXEL */
    { "White",				    kThemeBrushWhite, 0, 0 },					/*  46 */
    { "Menu",				    kThemeBrushMenuBackground, 0, 0 },				/*  47: MENU_BACKGROUND_PIXEL */
    { "DialogBackgroundActive",		    kThemeBrushDialogBackgroundActive, 0, 0 },			/*  48 */
    { "MenuDisabled",			    0, kThemeTextColorMenuItemDisabled, 0 },			/*  49: MENU_DISABLED_PIXEL */
    { "DialogBackgroundInactive",	    kThemeBrushDialogBackgroundInactive, 0, 0 },		/*  50 */
    { "MenuText",			    0, kThemeTextColorMenuItemActive, 0 },			/*  51: MENU_TEXT_PIXEL */
    { "AppearanceColor",		    0, 0, 0 },							/*  52: APPEARANCE_PIXEL */
    { "AlertBackgroundActive",		    kThemeBrushAlertBackgroundActive, 0, 0 },			/*  53 */
    { "AlertBackgroundInactive",	    kThemeBrushAlertBackgroundInactive, 0, 0 },			/*  54 */
    { "ModelessDialogBackgroundActive",	    kThemeBrushModelessDialogBackgroundActive, 0, 0 },		/*  55 */
    { "ModelessDialogBackgroundInactive",   kThemeBrushModelessDialogBackgroundInactive, 0, 0 },	/*  56 */
    { "UtilityWindowBackgroundActive",	    kThemeBrushUtilityWindowBackgroundActive, 0, 0 },		/*  57 */
    { "UtilityWindowBackgroundInactive",    kThemeBrushUtilityWindowBackgroundInactive, 0, 0 },		/*  58 */
    { "ListViewSortColumnBackground",	    kThemeBrushListViewSortColumnBackground, 0, 0 },		/*  59 */
    { "ListViewBackground",		    kThemeBrushListViewBackground, 0, 0 },			/*  60 */
    { "IconLabelBackground",		    kThemeBrushIconLabelBackground, 0, 0 },			/*  61 */
    { "ListViewSeparator",		    kThemeBrushListViewSeparator, 0, 0 },			/*  62 */
    { "ChasingArrows",			    kThemeBrushChasingArrows, 0, 0 },				/*  63 */
    { "DragHilite",			    kThemeBrushDragHilite, 0, 0 },				/*  64 */
    { "DocumentWindowBackground",	    kThemeBrushDocumentWindowBackground, 0, 0 },		/*  65 */
    { "FinderWindowBackground",		    kThemeBrushFinderWindowBackground, 0, 0 },			/*  66 */
    { "ScrollBarDelimiterActive",	    kThemeBrushScrollBarDelimiterActive, 0, 0 },		/*  67 */
    { "ScrollBarDelimiterInactive",	    kThemeBrushScrollBarDelimiterInactive, 0, 0 },		/*  68 */
    { "FocusHighlight",			    kThemeBrushFocusHighlight, 0, 0 },				/*  69 */
    { "PopupArrowActive",		    kThemeBrushPopupArrowActive, 0, 0 },			/*  70 */
    { "PopupArrowPressed",		    kThemeBrushPopupArrowPressed, 0, 0 },			/*  71 */
    { "PopupArrowInactive",		    kThemeBrushPopupArrowInactive, 0, 0 },			/*  72 */
    { "AppleGuideCoachmark",		    kThemeBrushAppleGuideCoachmark, 0, 0 },			/*  73 */
    { "IconLabelBackgroundSelected",	    kThemeBrushIconLabelBackgroundSelected, 0, 0 },		/*  74 */
    { "StaticAreaFill",			    kThemeBrushStaticAreaFill, 0, 0 },				/*  75 */
    { "ActiveAreaFill",			    kThemeBrushActiveAreaFill, 0, 0 },				/*  76 */
    { "ButtonFrameActive",		    kThemeBrushButtonFrameActive, 0, 0 },			/*  77 */
    { "ButtonFrameInactive",		    kThemeBrushButtonFrameInactive, 0, 0 },			/*  78 */
    { "ButtonFaceActive",		    kThemeBrushButtonFaceActive, 0, 0 },			/*  79 */
    { "ButtonFaceInactive",		    kThemeBrushButtonFaceInactive, 0, 0 },			/*  80 */
    { "ButtonFacePressed",		    kThemeBrushButtonFacePressed, 0, 0 },			/*  81 */
    { "ButtonActiveDarkShadow",		    kThemeBrushButtonActiveDarkShadow, 0, 0 },			/*  82 */
    { "ButtonActiveDarkHighlight",	    kThemeBrushButtonActiveDarkHighlight, 0, 0 },		/*  83 */
    { "ButtonActiveLightShadow",	    kThemeBrushButtonActiveLightShadow, 0, 0 },			/*  84 */
    { "ButtonActiveLightHighlight",	    kThemeBrushButtonActiveLightHighlight, 0, 0 },		/*  85 */
    { "ButtonInactiveDarkShadow",	    kThemeBrushButtonInactiveDarkShadow, 0, 0 },		/*  86 */
    { "ButtonInactiveDarkHighlight",	    kThemeBrushButtonInactiveDarkHighlight, 0, 0 },		/*  87 */
    { "ButtonInactiveLightShadow",	    kThemeBrushButtonInactiveLightShadow, 0, 0 },		/*  88 */
    { "ButtonInactiveLightHighlight",	    kThemeBrushButtonInactiveLightHighlight, 0, 0 },		/*  89 */
    { "ButtonPressedDarkShadow",	    kThemeBrushButtonPressedDarkShadow, 0, 0 },			/*  90 */
    { "ButtonPressedDarkHighlight",	    kThemeBrushButtonPressedDarkHighlight, 0, 0 },		/*  91 */
    { "ButtonPressedLightShadow",	    kThemeBrushButtonPressedLightShadow, 0, 0 },		/*  92 */
    { "ButtonPressedLightHighlight",	    kThemeBrushButtonPressedLightHighlight, 0, 0 },		/*  93 */
    { "BevelActiveLight",		    kThemeBrushBevelActiveLight, 0, 0 },			/*  94 */
    { "BevelActiveDark",		    kThemeBrushBevelActiveDark, 0, 0 },				/*  95 */
    { "BevelInactiveLight",		    kThemeBrushBevelInactiveLight, 0, 0 },			/*  96 */
    { "BevelInactiveDark",		    kThemeBrushBevelInactiveDark, 0, 0 },			/*  97 */
    { "NotificationWindowBackground",	    kThemeBrushNotificationWindowBackground, 0, 0 },		/*  98 */
    { "MovableModalBackground",		    kThemeBrushMovableModalBackground, 0, 0 },			/*  99 */
    { "SheetBackgroundOpaque",		    kThemeBrushSheetBackgroundOpaque, 0, 0 },			/* 100 */
    { "DrawerBackground",		    kThemeBrushDrawerBackground, 0, 0 },			/* 101 */
    { "ToolbarBackground",		    kThemeBrushToolbarBackground, 0, 0 },			/* 102 */
    { "SheetBackgroundTransparent",	    kThemeBrushSheetBackgroundTransparent, 0, 0 },		/* 103 */
    { "MenuBackground",			    kThemeBrushMenuBackground, 0, 0 },				/* 104 */
    { "Pixel",				    0, 0, 0 },							/* 105: PIXEL_MAGIC */
    { "MenuBackgroundSelected",		    kThemeBrushMenuBackgroundSelected, 0, 0 },			/* 106 */
    { "ListViewOddRowBackground",	    kThemeBrushListViewOddRowBackground, 0, 0 },		/* 107 */
    { "ListViewEvenRowBackground",	    kThemeBrushListViewEvenRowBackground, 0, 0 },		/* 108 */
    { "ListViewColumnDivider",		    kThemeBrushListViewColumnDivider, 0, 0 },			/* 109 */
    { "BlackText",			    0, kThemeTextColorBlack, 0 },				/* 110 */
    { "DialogActiveText",		    0, kThemeTextColorDialogActive, 0 },			/* 111 */
    { "DialogInactiveText",		    0, kThemeTextColorDialogInactive, 0 },			/* 112 */
    { "AlertActiveText",		    0, kThemeTextColorAlertActive, 0 },				/* 113 */
    { "AlertInactiveText",		    0, kThemeTextColorAlertInactive, 0 },			/* 114 */
    { "ModelessDialogActiveText",	    0, kThemeTextColorModelessDialogActive, 0 },		/* 115 */
    { "ModelessDialogInactiveText",	    0, kThemeTextColorModelessDialogInactive, 0 },		/* 116 */
    { "WindowHeaderActiveText",		    0, kThemeTextColorWindowHeaderActive, 0 },			/* 117 */
    { "WindowHeaderInactiveText",	    0, kThemeTextColorWindowHeaderInactive, 0 },		/* 118 */
    { "PlacardActiveText",		    0, kThemeTextColorPlacardActive, 0 },			/* 119 */
    { "PlacardInactiveText",		    0, kThemeTextColorPlacardInactive, 0 },			/* 120 */
    { "PlacardPressedText",		    0, kThemeTextColorPlacardPressed, 0 },			/* 121 */
    { "PushButtonActiveText",		    0, kThemeTextColorPushButtonActive, 0 },			/* 122 */
    { "PushButtonInactiveText",		    0, kThemeTextColorPushButtonInactive, 0 },			/* 123 */
    { "PushButtonPressedText",		    0, kThemeTextColorPushButtonPressed, 0 },			/* 124 */
    { "BevelButtonActiveText",		    0, kThemeTextColorBevelButtonActive, 0 },			/* 125 */
    { "BevelButtonInactiveText",	    0, kThemeTextColorBevelButtonInactive, 0 },			/* 126 */
    { "BevelButtonPressedText",		    0, kThemeTextColorBevelButtonPressed, 0 },			/* 127 */
    { "PopupButtonActiveText",		    0, kThemeTextColorPopupButtonActive, 0 },			/* 128 */
    { "PopupButtonInactiveText",	    0, kThemeTextColorPopupButtonInactive, 0 },			/* 129 */
    { "PopupButtonPressedText",		    0, kThemeTextColorPopupButtonPressed, 0 },			/* 130 */
    { "IconLabelText",			    0, kThemeTextColorIconLabel, 0 },				/* 131 */
    { "ListViewText",			    0, kThemeTextColorListView, 0 },				/* 132 */
    { "DocumentWindowTitleActiveText",	    0, kThemeTextColorDocumentWindowTitleActive, 0 },		/* 133 */
    { "DocumentWindowTitleInactiveText",    0, kThemeTextColorDocumentWindowTitleInactive, 0 },		/* 134 */
    { "MovableModalWindowTitleActiveText",  0, kThemeTextColorMovableModalWindowTitleActive, 0 },	/* 135 */
    { "MovableModalWindowTitleInactiveText",0, kThemeTextColorMovableModalWindowTitleInactive, 0 },	/* 136 */
    { "UtilityWindowTitleActiveText",	    0, kThemeTextColorUtilityWindowTitleActive, 0 },		/* 137 */
    { "UtilityWindowTitleInactiveText",	    0, kThemeTextColorUtilityWindowTitleInactive, 0 },		/* 138 */
    { "PopupWindowTitleActiveText",	    0, kThemeTextColorPopupWindowTitleActive, 0 },		/* 139 */
    { "PopupWindowTitleInactiveText",	    0, kThemeTextColorPopupWindowTitleInactive, 0 },		/* 140 */
    { "RootMenuActiveText",		    0, kThemeTextColorRootMenuActive, 0 },			/* 141 */
    { "RootMenuSelectedText",		    0, kThemeTextColorRootMenuSelected, 0 },			/* 142 */
    { "RootMenuDisabledText",		    0, kThemeTextColorRootMenuDisabled, 0 },			/* 143 */
    { "MenuItemActiveText",		    0, kThemeTextColorMenuItemActive, 0 },			/* 144 */
    { "MenuItemSelectedText",		    0, kThemeTextColorMenuItemSelected, 0 },			/* 145 */
    { "MenuItemDisabledText",		    0, kThemeTextColorMenuItemDisabled, 0 },			/* 146 */
    { "PopupLabelActiveText",		    0, kThemeTextColorPopupLabelActive, 0 },			/* 147 */
    { "PopupLabelInactiveText",		    0, kThemeTextColorPopupLabelInactive, 0 },			/* 148 */
    { "TabFrontActiveText",		    0, kThemeTextColorTabFrontActive, 0 },			/* 149 */
    { "TabNonFrontActiveText",		    0, kThemeTextColorTabNonFrontActive, 0 },			/* 150 */
    { "TabNonFrontPressedText",		    0, kThemeTextColorTabNonFrontPressed, 0 },			/* 151 */
    { "TabFrontInactiveText",		    0, kThemeTextColorTabFrontInactive, 0 },			/* 152 */
    { "TabNonFrontInactiveText",	    0, kThemeTextColorTabNonFrontInactive, 0 },			/* 153 */
    { "IconLabelSelectedText",		    0, kThemeTextColorIconLabelSelected, 0 },			/* 154 */
    { "BevelButtonStickyActiveText",	    0, kThemeTextColorBevelButtonStickyActive, 0 },		/* 155 */
    { "BevelButtonStickyInactiveText",	    0, kThemeTextColorBevelButtonStickyInactive, 0 },		/* 156 */
    { "NotificationText",		    0, kThemeTextColorNotification, 0 },			/* 157 */
    { "SystemDetailText",		    0, kThemeTextColorSystemDetail, 0 },			/* 158 */
    { "WhiteText",			    0, kThemeTextColorWhite, 0 },				/* 159 */
    { "TabPaneBackground",		    0, 0, kThemeBackgroundTabPane },				/* 160 */
    { "PlacardBackground",		    0, 0, kThemeBackgroundPlacard },				/* 161 */
    { "WindowHeaderBackground",		    0, 0, kThemeBackgroundWindowHeader },			/* 162 */
    { "ListViewWindowHeaderBackground",	    0, 0, kThemeBackgroundListViewWindowHeader },		/* 163 */
    { "SecondaryGroupBoxBackground",	    0, 0, kThemeBackgroundSecondaryGroupBox },			/* 164 */
    { "MetalBackground",		    0, 0, kThemeBackgroundMetal },				/* 165 */
    { NULL,				    0, 0, 0 }
};
#define MAX_PIXELCODE 165

/*
 *----------------------------------------------------------------------
 *
 * GetThemeFromPixelCode --
 *
 *	When given a pixel code corresponding to a theme system color,
 *	set one of brush, textColor or background to the corresponding
 *	Appearance Mgr theme constant.
 *
 * Results:
 *	Returns false if not a real pixel, true otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetThemeFromPixelCode(
    unsigned char code,
    ThemeBrush *brush,
    ThemeTextColor *textColor,
    ThemeBackgroundKind *background)
{
    if (code >= MIN_PIXELCODE && code <= MAX_PIXELCODE) {
	*brush = systemColorMap[code - MIN_PIXELCODE].brush;
	*textColor = systemColorMap[code - MIN_PIXELCODE].textColor;
	*background = systemColorMap[code - MIN_PIXELCODE].background;
    } else {
	*brush = 0;
	*textColor = 0;
	*background = 0;
    }
    if (!*brush && !*textColor && !*background && code != PIXEL_MAGIC &&
	    code != TRANSPARENT_PIXEL) {
	return false;
    } else {
	return true;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetThemeColor --
 *
 *	Get RGB color for a given system color or pixel value.
 *
 * Results:
 *	OSStatus
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static OSStatus
GetThemeColor(
    unsigned long pixel,
    ThemeBrush brush,
    ThemeTextColor textColor,
    ThemeBackgroundKind background,
    CGColorRef *c)
{
    OSStatus err = noErr;

    if (brush) {
	err = ChkErr(HIThemeBrushCreateCGColor, brush, c);
    /*} else if (textColor) {
	err = ChkErr(GetThemeTextColor, textColor, 32, true, c);*/
    } else {
	CGFloat rgba[4] = {0, 0, 0, 1};

	switch ((pixel >> 24) & 0xff) {
	case PIXEL_MAGIC: {
	    unsigned short red, green, blue;
	    red		= (pixel >> 16) & 0xff;
	    green	= (pixel >>  8) & 0xff;
	    blue	= (pixel      ) & 0xff;
	    red		|= red   << 8;
	    green	|= green << 8;
	    blue	|= blue  << 8;
	    rgba[0]	= red	/ 65535.0;
	    rgba[1]	= green / 65535.0;
	    rgba[2]	= blue  / 65535.0;
	    break;
	    }
	case TRANSPARENT_PIXEL:
	    rgba[3]	= 0.0;
	    break;
	}

	static CGColorSpaceRef deviceRGBSpace = NULL;
	if (!deviceRGBSpace) {
	    deviceRGBSpace = CGColorSpaceCreateDeviceRGB();
	}
	*c = CGColorCreate(deviceRGBSpace, rgba );
    }
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetMacColor --
 *
 *	Creates a CGColorRef from a X style pixel value.
 *
 * Results:
 *	Returns false if not a real pixel, true otherwise.
 *
 * Side effects:
 *	The variable macColor is set to a new CGColorRef, the caller is
 *	responsible for releasing it!
 *
 *----------------------------------------------------------------------
 */

int
TkSetMacColor(
    unsigned long pixel,		/* Pixel value to convert. */
    void *macColor)			/* CGColorRef to modify. */
{
    CGColorRef *color = (CGColorRef*)macColor;
    OSStatus err = -1;
    ThemeBrush brush;
    ThemeTextColor textColor;
    ThemeBackgroundKind background;

    if (GetThemeFromPixelCode((pixel >> 24) & 0xff, &brush, &textColor,
	    &background)) {
	err = ChkErr(GetThemeColor, pixel, brush, textColor, background,
		color);
    }
    return (err == noErr);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpInitGCCache, TkpFreeGCCache, CopyCachedColor, SetCachedColor --
 *
 *	Maintain a per-GC cache of previously converted CGColorRefs
 *
 * Results:
 *	None resp. retained CGColorRef for CopyCachedColor()
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpInitGCCache(
    GC gc)
{
    bzero(TkpGetGCCache(gc), sizeof(TkpGCCache));
}

void
TkpFreeGCCache(
    GC gc)
{
    TkpGCCache *gcCache = TkpGetGCCache(gc);

    if (gcCache->cachedForegroundColor) {
	CFRelease(gcCache->cachedForegroundColor);
    }
    if (gcCache->cachedBackgroundColor) {
	CFRelease(gcCache->cachedBackgroundColor);
    }
}

static CGColorRef
CopyCachedColor(
    GC gc,
    unsigned long pixel)
{
    TkpGCCache *gcCache = TkpGetGCCache(gc);
    CGColorRef cgColor = NULL;

    if (gcCache) {
	if (gcCache->cachedForeground == pixel) {
	    cgColor = gcCache->cachedForegroundColor;
	} else if (gcCache->cachedBackground == pixel) {
	    cgColor = gcCache->cachedBackgroundColor;
	}
	if (cgColor) {
	    CFRetain(cgColor);
	}
    }
    return cgColor;
}

static void
SetCachedColor(
    GC gc,
    unsigned long pixel,
    CGColorRef cgColor)
{
    TkpGCCache *gcCache = TkpGetGCCache(gc);

    if (gcCache && cgColor) {
	if (gc->foreground == pixel) {
	    if (gcCache->cachedForegroundColor) {
		CFRelease(gcCache->cachedForegroundColor);
	    }
	    gcCache->cachedForegroundColor = (CGColorRef) CFRetain(cgColor);
	    gcCache->cachedForeground = pixel;
	} else if (gc->background == pixel) {
	    if (gcCache->cachedBackgroundColor) {
		CFRelease(gcCache->cachedBackgroundColor);
	    }
	    gcCache->cachedBackgroundColor = (CGColorRef) CFRetain(cgColor);
	    gcCache->cachedBackground = pixel;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXCreateCGColor --
 *
 *	Creates a CGColorRef from a X style pixel value.
 *
 * Results:
 *	Returns NULL if not a real pixel, CGColorRef otherwise.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

CGColorRef
TkMacOSXCreateCGColor(
    GC gc,
    unsigned long pixel)		/* Pixel value to convert. */
{
    CGColorRef cgColor = CopyCachedColor(gc, pixel);

    if (!cgColor && TkSetMacColor(pixel, &cgColor)) {
	SetCachedColor(gc, pixel, cgColor);
    }
    return cgColor;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetNSColor --
 *
 *	Creates an autoreleased NSColor from a X style pixel value.
 *
 * Results:
 *	Returns nil if not a real pixel, NSColor* otherwise.
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */

NSColor*
TkMacOSXGetNSColor(
    GC gc,
    unsigned long pixel)		/* Pixel value to convert. */
{
    CGColorRef cgColor = TkMacOSXCreateCGColor(gc, pixel);
    NSColor *nsColor = nil;

    if (cgColor) {
	NSColorSpace *colorSpace = [[NSColorSpace alloc]
		initWithCGColorSpace:CGColorGetColorSpace(cgColor)];
	nsColor = [NSColor colorWithColorSpace:colorSpace
		components:CGColorGetComponents(cgColor)
		count:CGColorGetNumberOfComponents(cgColor)];
	[colorSpace release];
	CFRelease(cgColor);
    }
    return nsColor;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXSetColorInContext --
 *
 *	Sets fill and stroke color in the given CG context from an X
 *	pixel value, or if the pixel code indicates a system color,
 *	sets the corresponding brush, textColor or background via
 *	HITheme APIs if available or Appearance mgr APIs.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXSetColorInContext(
    GC gc,
    unsigned long pixel,
    CGContextRef context)
{
    OSStatus err = -1;
    CGColorRef cgColor = CopyCachedColor(gc, pixel);
    ThemeBrush brush;
    ThemeTextColor textColor;
    ThemeBackgroundKind background;

    if (!cgColor && GetThemeFromPixelCode((pixel >> 24) & 0xff, &brush,
	    &textColor, &background)) {
	if (brush) {
	    err = ChkErr(HIThemeSetFill, brush, NULL, context,
		    kHIThemeOrientationNormal);
	    if (err == noErr) {
		err = ChkErr(HIThemeSetStroke, brush, NULL, context,
			kHIThemeOrientationNormal);
	    }
	} else if (textColor) {
	    err = ChkErr(HIThemeSetTextFill, textColor, NULL, context,
		    kHIThemeOrientationNormal);
	} else if (background) {
	    CGRect rect = CGContextGetClipBoundingBox(context);
	    HIThemeBackgroundDrawInfo info = { 0, kThemeStateActive,
		    background };

	    err = ChkErr(HIThemeApplyBackground, &rect, &info,
		    context, kHIThemeOrientationNormal);
	}
	if (err == noErr) {
	    return;
	}
	err = ChkErr(GetThemeColor, pixel, brush, textColor, background,
		&cgColor);
	if (err == noErr) {
	    SetCachedColor(gc, pixel, cgColor);
	}
    } else if (!cgColor) {
	TkMacOSXDbgMsg("Ignored unknown pixel value 0x%lx", pixel);
    }
    if (cgColor) {
	CGContextSetFillColorWithColor(context, cgColor);
	CGContextSetStrokeColorWithColor(context, cgColor);
	CGColorRelease(cgColor);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColor --
 *
 *	Allocate a new TkColor for the color with the given name.
 *
 * Results:
 *	Returns a newly allocated TkColor, or NULL on failure.
 *
 * Side effects:
 *	May invalidate the colormap cache associated with tkwin upon
 *	allocating a new colormap entry. Allocates a new TkColor
 *	structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColor(
    Tk_Window tkwin,		/* Window in which color will be used. */
    Tk_Uid name)		/* Name of color to allocated (in form
				 * suitable for passing to XParseColor). */
{
    Display *display = tkwin != None ? Tk_Display(tkwin) : NULL;
    Colormap colormap = tkwin!= None ? Tk_Colormap(tkwin) : None;
    TkColor *tkColPtr;
    XColor color;

    /*
     * Check to see if this is a system color. Otherwise, XParseColor
     * will do all the work.
     */
    if (strncasecmp(name, "system", 6) == 0) {
	Tcl_Obj *strPtr = Tcl_NewStringObj(name+6, -1);
	int idx, result;

	result = Tcl_GetIndexFromObjStruct(NULL, strPtr, systemColorMap,
		    sizeof(struct SystemColorMapEntry), NULL, TCL_EXACT, &idx);
	Tcl_DecrRefCount(strPtr);
	if (result == TCL_OK) {
	    OSStatus err;
	    CGColorRef c;
	    unsigned char pixelCode = idx + MIN_PIXELCODE;
	    ThemeBrush brush = systemColorMap[idx].brush;
	    ThemeTextColor textColor = systemColorMap[idx].textColor;
	    ThemeBackgroundKind background = systemColorMap[idx].background;

	    err = ChkErr(GetThemeColor, 0, brush, textColor, background, &c);
	    if (err == noErr) {
		const size_t n = CGColorGetNumberOfComponents(c);
		const CGFloat *rgba = CGColorGetComponents(c);

		switch (n) {
		case 4:
		    color.red   = rgba[0] * 65535.0;
		    color.green = rgba[1] * 65535.0;
		    color.blue  = rgba[2] * 65535.0;
		    break;
		case 2:
		    color.red = color.green = color.blue = rgba[0] * 65535.0;
		    break;
		default:
		  Tcl_Panic("CGColor with %d components", (int) n);
		}
		color.pixel = ((((((pixelCode << 8)
		    | ((color.red   >> 8) & 0xff)) << 8)
		    | ((color.green >> 8) & 0xff)) << 8)
		    | ((color.blue  >> 8) & 0xff));
		CGColorRelease(c);
		goto validXColor;
	    }
	    CGColorRelease(c);
	}
    }

    if (TkParseColor(display, colormap, name, &color) == 0) {
	return NULL;
    }

validXColor:
    tkColPtr = ckalloc(sizeof(TkColor));
    tkColPtr->color = color;

    return tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetColorByValue --
 *
 *	Given a desired set of red-green-blue intensities for a color,
 *	locate a pixel value to use to draw that color in a given
 *	window.
 *
 * Results:
 *	The return value is a pointer to an TkColor structure that
 *	indicates the closest red, blue, and green intensities available
 *	to those specified in colorPtr, and also specifies a pixel
 *	value to use to draw in that color.
 *
 * Side effects:
 *	May invalidate the colormap cache for the specified window.
 *	Allocates a new TkColor structure.
 *
 *----------------------------------------------------------------------
 */

TkColor *
TkpGetColorByValue(
    Tk_Window tkwin,		/* Window in which color will be used. */
    XColor *colorPtr)		/* Red, green, and blue fields indicate
				 * desired color. */
{
    TkColor *tkColPtr = ckalloc(sizeof(TkColor));

    tkColPtr->color.red = colorPtr->red;
    tkColPtr->color.green = colorPtr->green;
    tkColPtr->color.blue = colorPtr->blue;
    tkColPtr->color.pixel = TkpGetPixel(&tkColPtr->color);
    return tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Stub functions --
 *
 *	These functions are just stubs for functions that either
 *	don't make sense on the Mac or have yet to be implemented.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	These calls do nothing - which may not be expected.
 *
 *----------------------------------------------------------------------
 */

Status
XAllocColor(
    Display *display,		/* Display. */
    Colormap map,		/* Not used. */
    XColor *colorPtr)		/* XColor struct to modify. */
{
    display->request++;
    colorPtr->pixel = TkpGetPixel(colorPtr);
    return 1;
}

Colormap
XCreateColormap(
    Display *display,		/* Display. */
    Window window,		/* X window. */
    Visual *visual,		/* Not used. */
    int alloc)			/* Not used. */
{
    static Colormap index = 1;

    /*
     * Just return a new value each time.
     */
    return index++;
}

int
XFreeColormap(
    Display* display,		/* Display. */
    Colormap colormap)		/* Colormap. */
{
    return Success;
}

int
XFreeColors(
    Display* display,		/* Display. */
    Colormap colormap,		/* Colormap. */
    unsigned long* pixels,	/* Array of pixels. */
    int npixels,		/* Number of pixels. */
    unsigned long planes)	/* Number of pixel planes. */
{
    /*
     * The Macintosh version of Tk uses TrueColor. Nothing
     * needs to be done to release colors as there really is
     * no colormap in the Tk sense.
     */
    return Success;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
