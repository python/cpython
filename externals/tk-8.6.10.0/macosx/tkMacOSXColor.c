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

/*
 * The colorType specifies how the color value should be interpreted.  For the
 * unique rgbColor entry, the RGB values are generated from the pixel value of
 * an XColor.  The ttkBackground and semantic types are dynamic, meaning
 * that they change when dark mode is enabled on OSX 10.13 and later.
 */

enum colorType {
    clearColor,    /* There should be only one of these. */
    rgbColor,      /* There should be only one of these. */
    appearance,    /* There should be only one of these. */
    HIBrush,       /* The value is a HITheme brush color table index. */
    HIText,        /* The value is a HITheme text color table index. */
    HIBackground,  /* The value is a HITheme background color table index. */
    ttkBackground, /* The value can be used as a parameter.*/
    semantic, /* The value can be used as a parameter.*/
};

/*

 */

struct SystemColorMapEntry {
    const char *name;
    enum colorType type;
    long value;
};  /* unsigned char pixelCode; */

/*
 * Array of system color definitions: the array index is required to equal the
 * color's (pixelCode - MIN_PIXELCODE), i.e. the array order needs to be kept
 * in sync with the public pixel code values in tkMacOSXPort.h !
 */

#define MIN_PIXELCODE  30
static const struct SystemColorMapEntry systemColorMap[] = {
    { "Transparent",			    clearColor,   0 },						    /*  30: TRANSPARENT_PIXEL */
    { "Highlight",			    HIBrush,      kThemeBrushPrimaryHighlightColor },		    /*  31 */
    { "HighlightSecondary",		    HIBrush,      kThemeBrushSecondaryHighlightColor },		    /*  32 */
    { "HighlightText",			    HIBrush,      kThemeBrushBlack },				    /*  33 */
    { "HighlightAlternate",		    HIBrush,      kThemeBrushAlternatePrimaryHighlightColor },	    /*  34 */
    { "ButtonText",			    HIText,       kThemeTextColorPushButtonActive },	       	    /*  35 */
    { "PrimaryHighlightColor",		    HIBrush,      kThemeBrushPrimaryHighlightColor },		    /*  36 */
    { "ButtonFace",			    HIBrush,      kThemeBrushButtonFaceActive },	       	    /*  37 */
    { "SecondaryHighlightColor",	    HIBrush,      kThemeBrushSecondaryHighlightColor },		    /*  38 */
    { "ButtonFrame",			    HIBrush,      kThemeBrushButtonFrameActive },	       	    /*  39 */
    { "AlternatePrimaryHighlightColor",	    HIBrush,      kThemeBrushAlternatePrimaryHighlightColor },	    /*  40 */
    { "WindowBody",			    HIBrush,      kThemeBrushDocumentWindowBackground },       	    /*  41 */
    { "SheetBackground",		    HIBrush,      kThemeBrushSheetBackground },			    /*  42 */
    { "MenuActive",			    HIBrush,      kThemeBrushMenuBackgroundSelected },		    /*  43 */
    { "Black",				    HIBrush,      kThemeBrushBlack },				    /*  44 */
    { "MenuActiveText",			    HIText,       kThemeTextColorMenuItemSelected },	       	    /*  45 */
    { "White",				    HIBrush,      kThemeBrushWhite },				    /*  46 */
    { "Menu",				    HIBrush,      kThemeBrushMenuBackground },			    /*  47 */
    { "DialogBackgroundActive",		    HIBrush,      kThemeBrushDialogBackgroundActive },		    /*  48 */
    { "MenuDisabled",			    HIText,       kThemeTextColorMenuItemDisabled },	       	    /*  49 */
    { "DialogBackgroundInactive",	    HIBrush,      kThemeBrushDialogBackgroundInactive },       	    /*  50 */
    { "MenuText",			    HIText,       kThemeTextColorMenuItemActive },	       	    /*  51 */
    { "AppearanceColor",		    appearance,   0 },						    /*  52: APPEARANCE_PIXEL */
    { "AlertBackgroundActive",		    HIBrush,      kThemeBrushAlertBackgroundActive },		    /*  53 */
    { "AlertBackgroundInactive",	    HIBrush,      kThemeBrushAlertBackgroundInactive },		    /*  54 */
    { "ModelessDialogBackgroundActive",	    HIBrush,      kThemeBrushModelessDialogBackgroundActive },	    /*  55 */
    { "ModelessDialogBackgroundInactive",   HIBrush,      kThemeBrushModelessDialogBackgroundInactive },    /*  56 */
    { "UtilityWindowBackgroundActive",	    HIBrush,      kThemeBrushUtilityWindowBackgroundActive },	    /*  57 */
    { "UtilityWindowBackgroundInactive",    HIBrush,      kThemeBrushUtilityWindowBackgroundInactive },	    /*  58 */
    { "ListViewSortColumnBackground",	    HIBrush,      kThemeBrushListViewSortColumnBackground },	    /*  59 */
    { "ListViewBackground",		    HIBrush,      kThemeBrushListViewBackground },	       	    /*  60 */
    { "IconLabelBackground",		    HIBrush,      kThemeBrushIconLabelBackground },    		    /*  61 */
    { "ListViewSeparator",		    HIBrush,      kThemeBrushListViewSeparator },      		    /*  62 */
    { "ChasingArrows",			    HIBrush,      kThemeBrushChasingArrows },			    /*  63 */
    { "DragHilite",			    HIBrush,      kThemeBrushDragHilite },	       		    /*  64 */
    { "DocumentWindowBackground",	    HIBrush,      kThemeBrushDocumentWindowBackground },       	    /*  65 */
    { "FinderWindowBackground",		    HIBrush,      kThemeBrushFinderWindowBackground },		    /*  66 */
    { "ScrollBarDelimiterActive",	    HIBrush,      kThemeBrushScrollBarDelimiterActive },       	    /*  67 */
    { "ScrollBarDelimiterInactive",	    HIBrush,      kThemeBrushScrollBarDelimiterInactive },     	    /*  68 */
    { "FocusHighlight",			    HIBrush,      kThemeBrushFocusHighlight },			    /*  69 */
    { "PopupArrowActive",		    HIBrush,      kThemeBrushPopupArrowActive },	       	    /*  70 */
    { "PopupArrowPressed",		    HIBrush,      kThemeBrushPopupArrowPressed },	       	    /*  71 */
    { "PopupArrowInactive",		    HIBrush,      kThemeBrushPopupArrowInactive },	       	    /*  72 */
    { "AppleGuideCoachmark",		    HIBrush,      kThemeBrushAppleGuideCoachmark },	       	    /*  73 */
    { "IconLabelBackgroundSelected",	    HIBrush,      kThemeBrushIconLabelBackgroundSelected },    	    /*  74 */
    { "StaticAreaFill",			    HIBrush,      kThemeBrushStaticAreaFill },			    /*  75 */
    { "ActiveAreaFill",			    HIBrush,      kThemeBrushActiveAreaFill },			    /*  76 */
    { "ButtonFrameActive",		    HIBrush,      kThemeBrushButtonFrameActive },		    /*  77 */
    { "ButtonFrameInactive",		    HIBrush,      kThemeBrushButtonFrameInactive },	       	    /*  78 */
    { "ButtonFaceActive",		    HIBrush,      kThemeBrushButtonFaceActive },       		    /*  79 */
    { "ButtonFaceInactive",		    HIBrush,      kThemeBrushButtonFaceInactive },	       	    /*  80 */
    { "ButtonFacePressed",		    HIBrush,      kThemeBrushButtonFacePressed },	       	    /*  81 */
    { "ButtonActiveDarkShadow",		    HIBrush,      kThemeBrushButtonActiveDarkShadow },		    /*  82 */
    { "ButtonActiveDarkHighlight",	    HIBrush,      kThemeBrushButtonActiveDarkHighlight },	    /*  83 */
    { "ButtonActiveLightShadow",	    HIBrush,      kThemeBrushButtonActiveLightShadow },		    /*  84 */
    { "ButtonActiveLightHighlight",	    HIBrush,      kThemeBrushButtonActiveLightHighlight },	    /*  85 */
    { "ButtonInactiveDarkShadow",	    HIBrush,      kThemeBrushButtonInactiveDarkShadow },	    /*  86 */
    { "ButtonInactiveDarkHighlight",	    HIBrush,      kThemeBrushButtonInactiveDarkHighlight },	    /*  87 */
    { "ButtonInactiveLightShadow",	    HIBrush,      kThemeBrushButtonInactiveLightShadow },	    /*  88 */
    { "ButtonInactiveLightHighlight",	    HIBrush,      kThemeBrushButtonInactiveLightHighlight },	    /*  89 */
    { "ButtonPressedDarkShadow",	    HIBrush,      kThemeBrushButtonPressedDarkShadow },		    /*  90 */
    { "ButtonPressedDarkHighlight",	    HIBrush,      kThemeBrushButtonPressedDarkHighlight },	    /*  91 */
    { "ButtonPressedLightShadow",	    HIBrush,      kThemeBrushButtonPressedLightShadow },	    /*  92 */
    { "ButtonPressedLightHighlight",	    HIBrush,      kThemeBrushButtonPressedLightHighlight },	    /*  93 */
    { "BevelActiveLight",		    HIBrush,      kThemeBrushBevelActiveLight },		    /*  94 */
    { "BevelActiveDark",		    HIBrush,      kThemeBrushBevelActiveDark },			    /*  95 */
    { "BevelInactiveLight",		    HIBrush,      kThemeBrushBevelInactiveLight },		    /*  96 */
    { "BevelInactiveDark",		    HIBrush,      kThemeBrushBevelInactiveDark },		    /*  97 */
    { "NotificationWindowBackground",	    HIBrush,      kThemeBrushNotificationWindowBackground },	    /*  98 */
    { "MovableModalBackground",		    HIBrush,      kThemeBrushMovableModalBackground },		    /*  99 */
    { "SheetBackgroundOpaque",		    HIBrush,      kThemeBrushSheetBackgroundOpaque },		    /* 100 */
    { "DrawerBackground",		    HIBrush,      kThemeBrushDrawerBackground },		    /* 101 */
    { "ToolbarBackground",		    HIBrush,      kThemeBrushToolbarBackground },		    /* 102 */
    { "SheetBackgroundTransparent",	    HIBrush,      kThemeBrushSheetBackgroundTransparent },	    /* 103 */
    { "MenuBackground",			    HIBrush,      kThemeBrushMenuBackground },			    /* 104 */
    { "Pixel",				    rgbColor,     0 },						    /* 105: PIXEL_MAGIC */
    { "MenuBackgroundSelected",		    HIBrush,      kThemeBrushMenuBackgroundSelected },		    /* 106 */
    { "ListViewOddRowBackground",	    HIBrush,      kThemeBrushListViewOddRowBackground },	    /* 107 */
    { "ListViewEvenRowBackground",	    HIBrush,      kThemeBrushListViewEvenRowBackground },	    /* 108 */
    { "ListViewColumnDivider",		    HIBrush,      kThemeBrushListViewColumnDivider },		    /* 109 */
    { "BlackText",			    HIText,       kThemeTextColorBlack },			    /* 110 */
    { "DialogActiveText",		    HIText,       kThemeTextColorDialogActive },		    /* 111 */
    { "DialogInactiveText",		    HIText,       kThemeTextColorDialogInactive },		    /* 112 */
    { "AlertActiveText",		    HIText,       kThemeTextColorAlertActive },			    /* 113 */
    { "AlertInactiveText",		    HIText,       kThemeTextColorAlertInactive },		    /* 114 */
    { "ModelessDialogActiveText",	    HIText,       kThemeTextColorModelessDialogActive },	    /* 115 */
    { "ModelessDialogInactiveText",	    HIText,       kThemeTextColorModelessDialogInactive },	    /* 116 */
    { "WindowHeaderActiveText",		    HIText,       kThemeTextColorWindowHeaderActive },		    /* 117 */
    { "WindowHeaderInactiveText",	    HIText,       kThemeTextColorWindowHeaderInactive },	    /* 118 */
    { "PlacardActiveText",		    HIText,       kThemeTextColorPlacardActive },		    /* 119 */
    { "PlacardInactiveText",		    HIText,       kThemeTextColorPlacardInactive },		    /* 120 */
    { "PlacardPressedText",		    HIText,       kThemeTextColorPlacardPressed },		    /* 121 */
    { "PushButtonActiveText",		    HIText,       kThemeTextColorPushButtonActive },		    /* 122 */
    { "PushButtonInactiveText",		    HIText,       kThemeTextColorPushButtonInactive },		    /* 123 */
    { "PushButtonPressedText",		    HIText,       kThemeTextColorPushButtonPressed },		    /* 124 */
    { "BevelButtonActiveText",		    HIText,       kThemeTextColorBevelButtonActive },		    /* 125 */
    { "BevelButtonInactiveText",	    HIText,       kThemeTextColorBevelButtonInactive },		    /* 126 */
    { "BevelButtonPressedText",		    HIText,       kThemeTextColorBevelButtonPressed },		    /* 127 */
    { "PopupButtonActiveText",		    HIText,       kThemeTextColorPopupButtonActive },		    /* 128 */
    { "PopupButtonInactiveText",	    HIText,       kThemeTextColorPopupButtonInactive },		    /* 129 */
    { "PopupButtonPressedText",		    HIText,       kThemeTextColorPopupButtonPressed },		    /* 130 */
    { "IconLabelText",			    HIText,       kThemeTextColorIconLabel },			    /* 131 */
    { "ListViewText",			    HIText,       kThemeTextColorListView },			    /* 132 */
    { "DocumentWindowTitleActiveText",	    HIText,       kThemeTextColorDocumentWindowTitleActive },	    /* 133 */
    { "DocumentWindowTitleInactiveText",    HIText,       kThemeTextColorDocumentWindowTitleInactive },	    /* 134 */
    { "MovableModalWindowTitleActiveText",  HIText,       kThemeTextColorMovableModalWindowTitleActive },   /* 135 */
    { "MovableModalWindowTitleInactiveText",HIText,       kThemeTextColorMovableModalWindowTitleInactive }, /* 136 */
    { "UtilityWindowTitleActiveText",	    HIText,       kThemeTextColorUtilityWindowTitleActive },	    /* 137 */
    { "UtilityWindowTitleInactiveText",	    HIText,       kThemeTextColorUtilityWindowTitleInactive },	    /* 138 */
    { "PopupWindowTitleActiveText",	    HIText,       kThemeTextColorPopupWindowTitleActive },	    /* 139 */
    { "PopupWindowTitleInactiveText",	    HIText,       kThemeTextColorPopupWindowTitleInactive },	    /* 140 */
    { "RootMenuActiveText",		    HIText,       kThemeTextColorRootMenuActive },		    /* 141 */
    { "RootMenuSelectedText",		    HIText,       kThemeTextColorRootMenuSelected },		    /* 142 */
    { "RootMenuDisabledText",		    HIText,       kThemeTextColorRootMenuDisabled },		    /* 143 */
    { "MenuItemActiveText",		    HIText,       kThemeTextColorMenuItemActive },		    /* 144 */
    { "MenuItemSelectedText",		    HIText,       kThemeTextColorMenuItemSelected },		    /* 145 */
    { "MenuItemDisabledText",		    HIText,       kThemeTextColorMenuItemDisabled },		    /* 146 */
    { "PopupLabelActiveText",		    HIText,       kThemeTextColorPopupLabelActive },		    /* 147 */
    { "PopupLabelInactiveText",		    HIText,       kThemeTextColorPopupLabelInactive },		    /* 148 */
    { "TabFrontActiveText",		    HIText,       kThemeTextColorTabFrontActive },		    /* 149 */
    { "TabNonFrontActiveText",		    HIText,       kThemeTextColorTabNonFrontActive },		    /* 150 */
    { "TabNonFrontPressedText",		    HIText,       kThemeTextColorTabNonFrontPressed },		    /* 151 */
    { "TabFrontInactiveText",		    HIText,       kThemeTextColorTabFrontInactive },		    /* 152 */
    { "TabNonFrontInactiveText",	    HIText,       kThemeTextColorTabNonFrontInactive },		    /* 153 */
    { "IconLabelSelectedText",		    HIText,       kThemeTextColorIconLabelSelected },		    /* 154 */
    { "BevelButtonStickyActiveText",	    HIText,       kThemeTextColorBevelButtonStickyActive },	    /* 155 */
    { "BevelButtonStickyInactiveText",	    HIText,       kThemeTextColorBevelButtonStickyInactive },	    /* 156 */
    { "NotificationText",		    HIText,       kThemeTextColorNotification },		    /* 157 */
    { "SystemDetailText",		    HIText,       kThemeTextColorSystemDetail },		    /* 158 */
    { "WhiteText",			    HIText,       kThemeTextColorWhite },			    /* 159 */
    { "TabPaneBackground",		    HIBackground, kThemeBackgroundTabPane },			    /* 160 */
    { "PlacardBackground",		    HIBackground, kThemeBackgroundPlacard },			    /* 161 */
    { "WindowHeaderBackground",		    HIBackground, kThemeBackgroundWindowHeader },		    /* 162 */
    { "ListViewWindowHeaderBackground",	    HIBackground, kThemeBackgroundListViewWindowHeader },	    /* 163 */
    { "SecondaryGroupBoxBackground",	    HIBackground, kThemeBackgroundSecondaryGroupBox },		    /* 164 */
    { "MetalBackground",		    HIBackground, kThemeBackgroundMetal },			    /* 165 */

    /*
     * Colors based on "semantic" NSColors.
     */

    { "WindowBackgroundColor",		    ttkBackground, 0 },	    					    /* 166 */
    { "WindowBackgroundColor1",		    ttkBackground, 1 },						    /* 167 */
    { "WindowBackgroundColor2",		    ttkBackground, 2 },						    /* 168 */
    { "WindowBackgroundColor3",		    ttkBackground, 3 },						    /* 169 */
    { "WindowBackgroundColor4",		    ttkBackground, 4 },						    /* 170 */
    { "WindowBackgroundColor5",		    ttkBackground, 5 },						    /* 171 */
    { "WindowBackgroundColor6",		    ttkBackground, 6 },						    /* 172 */
    { "WindowBackgroundColor7",		    ttkBackground, 7 },						    /* 173 */
    { "TextColor",			    semantic, 0 },						    /* 174 */
    { "SelectedTextColor",		    semantic, 1 },						    /* 175 */
    { "LabelColor",			    semantic, 2 },						    /* 176 */
    { "ControlTextColor",      		    semantic, 3 },						    /* 177 */
    { "DisabledControlTextColor",	    semantic, 4 },						    /* 178 */
    { "SelectedTabTextColor",		    semantic, 5 },						    /* 179 */
    { "TextBackgroundColor",		    semantic, 6 },						    /* 180 */
    { "SelectedTextBackgroundColor",	    semantic, 7 },						    /* 181 */
    { "ControlAccentColor",		    semantic, 8 },						    /* 182 */
    { NULL,				    0, 0 }
};
#define FIRST_SEMANTIC_COLOR 166
#define MAX_PIXELCODE 182

/*
 *----------------------------------------------------------------------
 *
 * GetEntryFromPixelCode --
 *
 *	Extract a SystemColorMapEntry from the table.
 *
 * Results:
 *	Returns false if the code is out of bounds.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static bool
GetEntryFromPixelCode(
    unsigned char code,
    struct SystemColorMapEntry *entry)
{
    if (code >= MIN_PIXELCODE && code <= MAX_PIXELCODE) {
	*entry = systemColorMap[code - MIN_PIXELCODE];
	return true;
    } else {
	return false;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetCGColorComponents --
 *
 *	Set the components of a CGColorRef from an XColor pixel value and a
 *      system color map entry.  The pixel value is only used in the case where
 *      the color is of type rgbColor.  In that case the normalized XColor RGB
 *      values are copied into the CGColorRef.
 *
 * Results:
 *	OSStatus
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static NSColorSpace* deviceRGB = NULL;
static CGFloat blueAccentRGBA[4] = {0, 122.0 / 255, 1.0, 1.0};
static CGFloat windowBackground[4] =
    {236.0 / 255, 236.0 / 255, 236.0 / 255, 1.0};

static OSStatus
SetCGColorComponents(
    struct SystemColorMapEntry entry,
    unsigned long pixel,
    CGColorRef *c)
{
    OSStatus err = noErr;
    NSColor *bgColor, *color = nil;
    CGFloat rgba[4] = {0, 0, 0, 1};
#if MAC_OS_X_VERSION_MAX_ALLOWED < 101400
    NSInteger colorVariant;
    static CGFloat graphiteAccentRGBA[4] =
	{152.0 / 255, 152.0 / 255, 152.0 / 255, 1.0};
#endif

    if (!deviceRGB) {
	deviceRGB = [NSColorSpace deviceRGBColorSpace];
    }

    /*
     * This function is called before our autorelease pool is set up,
     * so it needs its own pool.
     */

    NSAutoreleasePool *pool = [NSAutoreleasePool new];

    switch (entry.type) {
    case HIBrush:
	err = ChkErr(HIThemeBrushCreateCGColor, entry.value, c);
	return err;
    case rgbColor:
	rgba[0] = ((pixel >> 16) & 0xff) / 255.0;
	rgba[1] = ((pixel >>  8) & 0xff) / 255.0;
	rgba[2] = ((pixel      ) & 0xff) / 255.0;
	break;
    case ttkBackground:

	/*
	 * Prior to OSX 10.14, getComponents returns black when applied to
	 * windowBackGroundColor.
	 */

	if ([NSApp macMinorVersion] < 14) {
	    for (int i=0; i<3; i++) {
		rgba[i] = windowBackground[i];
	    }
	} else {
	    bgColor = [[NSColor windowBackgroundColor] colorUsingColorSpace:
			    deviceRGB];
	    [bgColor getComponents: rgba];
	}
	if (rgba[0] + rgba[1] + rgba[2] < 1.5) {
	    for (int i=0; i<3; i++) {
		rgba[i] += entry.value*8.0 / 255.0;
	    }
	} else {
	    for (int i=0; i<3; i++) {
		rgba[i] -= entry.value*8.0 / 255.0;
	    }
	}
	break;
    case semantic:
	switch (entry.value) {
	case 0:
	    color = [[NSColor textColor] colorUsingColorSpace: deviceRGB];
	    break;
	case 1:
	    color = [[NSColor selectedTextColor] colorUsingColorSpace: deviceRGB];
	    break;
	case 2:
	    if ([NSApp macMinorVersion] > 9) {
#if MAC_OS_X_VERSION_MAX_ALLOWED > 1090
		color = [[NSColor labelColor] colorUsingColorSpace: deviceRGB];
#endif
	    } else {
		color = [[NSColor textColor] colorUsingColorSpace: deviceRGB];
	    }
	    break;
	case 3:
	    color = [[NSColor controlTextColor] colorUsingColorSpace:
			  deviceRGB];
	    break;
	case 4:
	    color = [[NSColor disabledControlTextColor] colorUsingColorSpace:
			  deviceRGB];
	    break;
	case 5:
	    if ([NSApp macMinorVersion] > 6) {
		color = [[NSColor whiteColor] colorUsingColorSpace:
						  deviceRGB];
	    } else {
		color = [[NSColor blackColor] colorUsingColorSpace:
						  deviceRGB];
	    }
	    break;
	case 6:
	    color = [[NSColor textBackgroundColor] colorUsingColorSpace:
			  deviceRGB];
	    break;
	case 7:
	    color = [[NSColor selectedTextBackgroundColor] colorUsingColorSpace:
			  deviceRGB];
	    break;
	case 8:
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
	    if (@available(macOS 10.14, *)) {
		color = [[NSColor controlAccentColor] colorUsingColorSpace:
							  deviceRGB];
	    } else {
		color = [NSColor colorWithColorSpace: deviceRGB
				 components: blueAccentRGBA
				 count: 4];
	    }
#else
	    colorVariant = [[NSUserDefaults standardUserDefaults]
			       integerForKey:@"AppleAquaColorVariant"];
	    if (colorVariant == 6) {
		color = [NSColor colorWithColorSpace: deviceRGB
				 components: graphiteAccentRGBA
				 count: 4];
	    } else {
		color = [NSColor colorWithColorSpace: deviceRGB
				 components: blueAccentRGBA
				 count: 4];
	    }
#endif
	    break;
	default:
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101000
	    if ([NSApp macMinorVersion] >= 10) {
		color = [[NSColor labelColor] colorUsingColorSpace:
			      deviceRGB];
		break;
	    }
#endif
	    color = [[NSColor textColor] colorUsingColorSpace: deviceRGB];
	    break;
	}
	[color getComponents: rgba];
	break;
    case clearColor:
	rgba[3]	= 0.0;
	break;

    /*
     * There are no HITheme functions which convert Text or background colors
     * to CGColors.  (GetThemeTextColor has been removed, and it was never
     * possible with backgrounds.)  If we get one of these we return black.
     */

    case HIText:
    case HIBackground:
    default:
	break;
    }
    *c = CGColorCreate(deviceRGB.CGColorSpace, rgba);
    [pool drain];
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInDarkMode --
 *
 *      Tests whether the given window's NSView has a DarkAqua Appearance.
 *
 * Results:
 *      Returns true if the NSView is in DarkMode, false if not.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE Bool
TkMacOSXInDarkMode(Tk_Window tkwin)
{
    int result = false;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 101400
    if ([NSApp macMinorVersion] >= 14) {
        static NSAppearanceName darkAqua = @"NSAppearanceNameDarkAqua";
        TkWindow *winPtr = (TkWindow*) tkwin;
        NSView *view = TkMacOSXDrawableView(winPtr->privatePtr);
        result = (view &&
                  [view.effectiveAppearance.name isEqualToString:darkAqua]);
    }
#endif
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetMacColor --
 *
 *	Sets the components of a CGColorRef from an XColor pixel value.
 *      The high order byte of the pixel value is used as an index into
 *      the system color table, and then SetCGColorComponents is called
 *      with the table entry and the pixel value.
 *
 * Results:
 *      Returns false if the high order byte is not a valid index, true
 *	otherwise.
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
    struct SystemColorMapEntry entry;

    if (GetEntryFromPixelCode((pixel >> 24) & 0xff, &entry)) {
	err = ChkErr(SetCGColorComponents, entry, pixel, color);
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
 * Side effects:M
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
    OSStatus err = noErr;
    CGColorRef cgColor = nil;
    struct SystemColorMapEntry entry;
    CGRect rect;
    int code = (pixel >> 24) & 0xff;
    HIThemeBackgroundDrawInfo info = {0, kThemeStateActive, 0};;
    static CGColorSpaceRef deviceRGBSpace = NULL;

    if (!deviceRGBSpace) {
	deviceRGBSpace = CGColorSpaceCreateDeviceRGB();
    }
    if (code < FIRST_SEMANTIC_COLOR) {
	cgColor = CopyCachedColor(gc, pixel);
    }
    if (!cgColor && GetEntryFromPixelCode(code, &entry)) {
	switch (entry.type) {
	case HIBrush:
	    err = ChkErr(HIThemeSetFill, entry.value, NULL, context,
		    kHIThemeOrientationNormal);
	    if (err == noErr) {
		err = ChkErr(HIThemeSetStroke, entry.value, NULL, context,
			kHIThemeOrientationNormal);
	    }
	    break;
	case HIText:
	    err = ChkErr(HIThemeSetTextFill, entry.value, NULL, context,
		    kHIThemeOrientationNormal);
	    break;
	case HIBackground:
	    info.kind = entry.value;
	    rect = CGContextGetClipBoundingBox(context);
	    err = ChkErr(HIThemeApplyBackground, &rect, &info,
		    context, kHIThemeOrientationNormal);
	    break;
	default:
	    err = ChkErr(SetCGColorComponents, entry, pixel, &cgColor);
	    if (err == noErr) {
		SetCachedColor(gc, pixel, cgColor);
	    }
	    break;
	}
    }
    if (cgColor) {
	CGContextSetFillColorWithColor(context, cgColor);
	CGContextSetStrokeColorWithColor(context, cgColor);
	CGColorRelease(cgColor);
    }
    if (err != noErr) {
	TkMacOSXDbgMsg("Ignored unknown pixel value 0x%lx", pixel);
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
    Tk_Uid name)		/* Name of color to be allocated (in form
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
	    struct SystemColorMapEntry entry = systemColorMap[idx];

	    err = ChkErr(SetCGColorComponents, entry, 0, &c);
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
