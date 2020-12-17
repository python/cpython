/*
 * ttkMacOSXTheme.c --
 *
 *	Tk theme engine for Mac OSX, using the Appearance Manager API.
 *
 * Copyright (c) 2004 Joe English
 * Copyright (c) 2005 Neil Madden
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright 2008-2009, Apple Inc.
 * Copyright 2009 Kevin Walzer/WordTech Communications LLC.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * See also:
 *
 * <URL: http://developer.apple.com/documentation/Carbon/Reference/
 *	Appearance_Manager/appearance_manager/APIIndex.html >
 *
 * Notes:
 *	"Active" means different things in Mac and Tk terminology --
 *	On Aqua, widgets are "Active" if they belong to the foreground window,
 *	"Inactive" if they are in a background window.
 *	Tk uses the term "active" to mean that the mouse cursor
 *	is over a widget; aka "hover", "prelight", or "hot-tracked".
 *	Aqua doesn't use this kind of feedback.
 *
 *	The QuickDraw/Carbon coordinate system is relative to the
 *	top-level window, not to the Tk_Window.  BoxToRect()
 *	accounts for this.
 */

#include "tkMacOSXPrivate.h"
#include "ttk/ttkTheme.h"

/*
 * Use this version in the core:
 */
#define BEGIN_DRAWING(d) { \
	TkMacOSXDrawingContext dc; \
	if (!TkMacOSXSetupDrawingContext((d), NULL, 1, &dc)) {return;}
#define END_DRAWING \
	TkMacOSXRestoreDrawingContext(&dc); }

#define HIOrientation kHIThemeOrientationNormal

#ifdef __LP64__
#define RangeToFactor(maximum) (((double) (INT_MAX >> 1)) / (maximum))
#else
#define RangeToFactor(maximum) (((double) (LONG_MAX >> 1)) / (maximum))
#endif /* __LP64__ */

/*----------------------------------------------------------------------
 * +++ Utilities.
 */

/*
 * BoxToRect --
 *	Convert a Ttk_Box in Tk coordinates relative to the given Drawable
 *	to a native Rect relative to the containing port.
 */
static inline CGRect BoxToRect(Drawable d, Ttk_Box b)
{
    MacDrawable *md = (MacDrawable*)d;
    CGRect rect;

    rect.origin.y	= b.y + md->yOff;
    rect.origin.x	= b.x + md->xOff;
    rect.size.height	= b.height;
    rect.size.width	= b.width;

    return rect;
}

/*
 * Table mapping Tk states to Appearance manager ThemeStates
 */

static Ttk_StateTable ThemeStateTable[] = {
    {kThemeStateUnavailable, TTK_STATE_DISABLED, 0},
    {kThemeStatePressed, TTK_STATE_PRESSED, 0},
    {kThemeStateInactive, TTK_STATE_BACKGROUND, 0},
    {kThemeStateActive, 0, 0}
/* Others: Not sure what these are supposed to mean.
   Up/Down have something to do with "little arrow" increment controls...
   Dunno what a "Rollover" is.
   NEM: Rollover is TTK_STATE_ACTIVE... but we don't handle that yet, by the
   looks of things
    {kThemeStateRollover, 0, 0},
    {kThemeStateUnavailableInactive, 0, 0}
    {kThemeStatePressedUp, 0, 0},
    {kThemeStatePressedDown, 0, 0}
*/
};

/*----------------------------------------------------------------------
 * +++ Button element: Used for elements drawn with DrawThemeButton.
 */

/*
 * Extra margins to account for drop shadow.
 */
static Ttk_Padding ButtonMargins = {2,2,2,2};

#define NoThemeMetric 0xFFFFFFFF

typedef struct {
    ThemeButtonKind kind;
    ThemeMetric heightMetric;
} ThemeButtonParams;

static ThemeButtonParams
    PushButtonParams = { kThemePushButton, kThemeMetricPushButtonHeight },
    CheckBoxParams = { kThemeCheckBox, kThemeMetricCheckBoxHeight },
    RadioButtonParams = { kThemeRadioButton, kThemeMetricRadioButtonHeight },
    BevelButtonParams = { kThemeBevelButton, NoThemeMetric },
    PopupButtonParams = { kThemePopupButton, kThemeMetricPopupButtonHeight },
    DisclosureParams = { kThemeDisclosureButton, kThemeMetricDisclosureTriangleHeight },
    ListHeaderParams = { kThemeListHeaderButton, kThemeMetricListHeaderHeight };

static Ttk_StateTable ButtonValueTable[] = {
    { kThemeButtonMixed, TTK_STATE_ALTERNATE, 0 },
    { kThemeButtonOn, TTK_STATE_SELECTED, 0 },
    { kThemeButtonOff, 0, 0 }
/* Others: kThemeDisclosureRight, kThemeDisclosureDown, kThemeDisclosureLeft */
};

static Ttk_StateTable ButtonAdornmentTable[] = {
    { kThemeAdornmentDefault| kThemeAdornmentFocus,
	TTK_STATE_ALTERNATE| TTK_STATE_FOCUS, 0 },
    { kThemeAdornmentDefault, TTK_STATE_ALTERNATE, 0 },
    { kThemeAdornmentFocus, TTK_STATE_FOCUS, 0 },
    { kThemeAdornmentNone, 0, 0 }
};

/*
 * computeButtonDrawInfo --
 *	Fill in an appearance manager HIThemeButtonDrawInfo record.
 */
static inline HIThemeButtonDrawInfo computeButtonDrawInfo(
	ThemeButtonParams *params, Ttk_State state)
{
    const HIThemeButtonDrawInfo info = {
	.version = 0,
	.state = Ttk_StateTableLookup(ThemeStateTable, state),
	.kind = params ? params->kind : 0,
	.value = Ttk_StateTableLookup(ButtonValueTable, state),
	.adornment = Ttk_StateTableLookup(ButtonAdornmentTable, state),
    };
    return info;
}

static void ButtonElementSizeNoPadding(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ThemeButtonParams *params = clientData;

    if (params->heightMetric != NoThemeMetric) {
	SInt32 height;

	ChkErr(GetThemeMetric, params->heightMetric, &height);
	*heightPtr = height;
    }
}

static void ButtonElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    ThemeButtonParams *params = clientData;
    const HIThemeButtonDrawInfo info = computeButtonDrawInfo(params, 0);
    static const CGRect scratchBounds = {{0, 0}, {100, 100}};
    CGRect contentBounds;

    ButtonElementSizeNoPadding(
	clientData, elementRecord, tkwin,
	widthPtr, heightPtr, paddingPtr);

    /*
     * To compute internal padding, query the appearance manager
     * for the content bounds of a dummy rectangle, then use
     * the difference as the padding.
     */
    ChkErr(HIThemeGetButtonContentBounds,
	&scratchBounds, &info, &contentBounds);

    paddingPtr->left = CGRectGetMinX(contentBounds);
    paddingPtr->top = CGRectGetMinY(contentBounds);
    paddingPtr->right = CGRectGetMaxX(scratchBounds) - CGRectGetMaxX(contentBounds) + 1;
    paddingPtr->bottom = CGRectGetMaxY(scratchBounds) - CGRectGetMaxY(contentBounds);

    /*
     * Now add a little extra padding to account for drop shadows.
     * @@@ SHOULD: call GetThemeButtonBackgroundBounds() instead.
     */

    *paddingPtr = Ttk_AddPadding(*paddingPtr, ButtonMargins);
    *widthPtr += Ttk_PaddingWidth(ButtonMargins);
    *heightPtr += Ttk_PaddingHeight(ButtonMargins);
}

static void ButtonElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    ThemeButtonParams *params = clientData;
    CGRect bounds = BoxToRect(d, Ttk_PadBox(b, ButtonMargins));
    const HIThemeButtonDrawInfo info = computeButtonDrawInfo(params, state);

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawButton, &bounds, &info, dc.context, HIOrientation, NULL);
    END_DRAWING
}

static Ttk_ElementSpec ButtonElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    ButtonElementSize,
    ButtonElementDraw
};

/*----------------------------------------------------------------------
 * +++ Notebook elements.
 */


/* Tab position logic, c.f. ttkNotebook.c TabState() */

#define TTK_STATE_NOTEBOOK_FIRST	TTK_STATE_USER1
#define TTK_STATE_NOTEBOOK_LAST 	TTK_STATE_USER2
static Ttk_StateTable TabStyleTable[] = {
    { kThemeTabFrontInactive, TTK_STATE_SELECTED|TTK_STATE_BACKGROUND},
    { kThemeTabNonFrontInactive, TTK_STATE_BACKGROUND},
    { kThemeTabFrontUnavailable, TTK_STATE_DISABLED|TTK_STATE_SELECTED},
    { kThemeTabNonFrontUnavailable, TTK_STATE_DISABLED},
    { kThemeTabFront, TTK_STATE_SELECTED},
    { kThemeTabNonFrontPressed, TTK_STATE_PRESSED},
    { kThemeTabNonFront, 0}
};

static Ttk_StateTable TabAdornmentTable[] = {
    { kHIThemeTabAdornmentNone,
	    TTK_STATE_NOTEBOOK_FIRST|TTK_STATE_NOTEBOOK_LAST},
    {kHIThemeTabAdornmentTrailingSeparator, TTK_STATE_NOTEBOOK_FIRST},
    {kHIThemeTabAdornmentNone, TTK_STATE_NOTEBOOK_LAST},
    {kHIThemeTabAdornmentTrailingSeparator, 0 },
};

static Ttk_StateTable TabPositionTable[] = {
    { kHIThemeTabPositionOnly,
	    TTK_STATE_NOTEBOOK_FIRST|TTK_STATE_NOTEBOOK_LAST},
    { kHIThemeTabPositionFirst, TTK_STATE_NOTEBOOK_FIRST},
    { kHIThemeTabPositionLast, TTK_STATE_NOTEBOOK_LAST},
    { kHIThemeTabPositionMiddle, 0 },
};

/*
 * Apple XHIG Tab View Specifications:
 *
 * Control sizes: Tab views are available in regular, small, and mini sizes.
 * The tab height is fixed for each size, but you control the size of the pane
 * area. The tab heights for each size are listed below:
 *  - Regular size: 20 pixels.
 *  - Small: 17 pixels.
 *  - Mini: 15 pixels.
 *
 * Label spacing and fonts: The tab labels should be in a font that’s
 * proportional to the size of the tab view control. In addition, the label
 * should be placed so that there are equal margins of space before and after
 * it. The guidelines below provide the specifications you should use for tab
 * labels:
 *  - Regular size: System font. Center in tab, leaving 12 pixels on each side.
 *  - Small: Small system font. Center in tab, leaving 10 pixels on each side.
 *  - Mini: Mini system font. Center in tab, leaving 8 pixels on each side.
 *
 * Control spacing: Whether you decide to inset a tab view in a window or
 * extend its edges to the window sides and bottom, you should place the top
 * edge of the tab view 12 or 14 pixels below the bottom edge of the title bar
 * (or toolbar, if there is one). If you choose to inset a tab view in a
 * window, you should leave a margin of 20 pixels between the sides and bottom
 * of the tab view and the sides and bottom of the window (although 16 pixels
 * is also an acceptable margin-width). If you need to provide controls below
 * the tab view, leave enough space below the tab view so the controls are 20
 * pixels above the bottom edge of the window and 12 pixels between the tab
 * view and the controls.
 * If you choose to extend the tab view sides and bottom so that they meet the
 * window sides and bottom, you should leave a margin of at least 20 pixels
 * between the content in the tab view and the tab-view edges.
 *
 * <URL: http://developer.apple.com/documentation/userexperience/Conceptual/
 *       AppleHIGuidelines/XHIGControls/XHIGControls.html#//apple_ref/doc/uid/
 *       TP30000359-TPXREF116>
 */

static void TabElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    GetThemeMetric(kThemeMetricLargeTabHeight, (SInt32 *)heightPtr);
    *paddingPtr = Ttk_MakePadding(0, 0, 0, 2);

}

static void TabElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    CGRect bounds = BoxToRect(d, b);
    HIThemeTabDrawInfo info = {
	.version = 1,
	.style = Ttk_StateTableLookup(TabStyleTable, state),
	.direction = kThemeTabNorth,
	.size = kHIThemeTabSizeNormal,
	.adornment = Ttk_StateTableLookup(TabAdornmentTable, state),
	.kind = kHIThemeTabKindNormal,
	.position = Ttk_StateTableLookup(TabPositionTable, state),
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawTab, &bounds, &info, dc.context, HIOrientation, NULL);
    END_DRAWING
}

static Ttk_ElementSpec TabElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    TabElementSize,
    TabElementDraw
};

/*
 * Notebook panes:
 */
static void PaneElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *paddingPtr = Ttk_MakePadding(9, 5, 9, 9);
}

static void PaneElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    CGRect bounds = BoxToRect(d, b);
    HIThemeTabPaneDrawInfo info = {
	.version = 1,
	.state = Ttk_StateTableLookup(ThemeStateTable, state),
	.direction = kThemeTabNorth,
	.size = kHIThemeTabSizeNormal,
	.kind = kHIThemeTabKindNormal,
	.adornment = kHIThemeTabPaneAdornmentNormal,
    };

    bounds.origin.y -= kThemeMetricTabFrameOverlap;
    bounds.size.height += kThemeMetricTabFrameOverlap;
    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawTabPane, &bounds, &info, dc.context, HIOrientation);
    END_DRAWING
}

static Ttk_ElementSpec PaneElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    PaneElementSize,
    PaneElementDraw
};

/*
 * Labelframe borders:
 * Use "primary group box ..."
 * Quoth DrawThemePrimaryGroup reference:
 * "The primary group box frame is drawn inside the specified
 * rectangle and is a maximum of 2 pixels thick."
 *
 * "Maximum of 2 pixels thick" is apparently a lie;
 * looks more like 4 to me with shading.
 */
static void GroupElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *paddingPtr = Ttk_UniformPadding(4);
}

static void GroupElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    CGRect bounds = BoxToRect(d, b);
    const HIThemeGroupBoxDrawInfo info = {
	.version = 0,
	.state = Ttk_StateTableLookup(ThemeStateTable, state),
	.kind = kHIThemeGroupBoxKindPrimaryOpaque,
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawGroupBox, &bounds, &info, dc.context, HIOrientation);
    END_DRAWING
}

static Ttk_ElementSpec GroupElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    GroupElementSize,
    GroupElementDraw
};

/*----------------------------------------------------------------------
 * +++ Entry element --
 *	3 pixels padding for focus rectangle
 *	2 pixels padding for EditTextFrame
 */

typedef struct {
    Tcl_Obj	*backgroundObj;
} EntryElement;

static Ttk_ElementOptionSpec EntryElementOptions[] = {
    { "-background", TK_OPTION_BORDER,
	    Tk_Offset(EntryElement,backgroundObj), "white" },
    {0}
};

static void EntryElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *paddingPtr = Ttk_UniformPadding(5);
}

static void EntryElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    EntryElement *e = elementRecord;
    Tk_3DBorder backgroundPtr = Tk_Get3DBorderFromObj(tkwin,e->backgroundObj);
    Ttk_Box inner = Ttk_PadBox(b, Ttk_UniformPadding(3));
    CGRect bounds = BoxToRect(d, inner);
    const HIThemeFrameDrawInfo info = {
	.version = 0,
	.kind = kHIThemeFrameTextFieldSquare,
	.state = Ttk_StateTableLookup(ThemeStateTable, state),
	.isFocused = state & TTK_STATE_FOCUS,
    };

    /*
     * Erase w/background color:
     */
    XFillRectangle(Tk_Display(tkwin), d,
	    Tk_3DBorderGC(tkwin, backgroundPtr, TK_3D_FLAT_GC),
	    inner.x,inner.y, inner.width, inner.height);

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawFrame, &bounds, &info, dc.context, HIOrientation);
    /*if (state & TTK_STATE_FOCUS) {
	ChkErr(DrawThemeFocusRect, &bounds, 1);
    }*/
    END_DRAWING
}

static Ttk_ElementSpec EntryElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(EntryElement),
    EntryElementOptions,
    EntryElementSize,
    EntryElementDraw
};

/*----------------------------------------------------------------------
 * +++ Combobox:
 *
 * NOTES:
 *	kThemeMetricComboBoxLargeDisclosureWidth -> 17
 *	Padding and margins guesstimated by trial-and-error.
 */

static Ttk_Padding ComboboxPadding = { 2, 3, 17, 1 };
static Ttk_Padding ComboboxMargins = { 3, 3, 4, 4 };

static void ComboboxElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *widthPtr = 0;
    *heightPtr = 0;
    *paddingPtr = Ttk_AddPadding(ComboboxMargins, ComboboxPadding);
}

static void ComboboxElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    CGRect bounds = BoxToRect(d, Ttk_PadBox(b, ComboboxMargins));
    const HIThemeButtonDrawInfo info = {
	.version = 0,
	.state = Ttk_StateTableLookup(ThemeStateTable, state),
	.kind = kThemeComboBox,
	.value = Ttk_StateTableLookup(ButtonValueTable, state),
	.adornment = Ttk_StateTableLookup(ButtonAdornmentTable, state),
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawButton, &bounds, &info, dc.context, HIOrientation, NULL);
    END_DRAWING
}

static Ttk_ElementSpec ComboboxElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    ComboboxElementSize,
    ComboboxElementDraw
};





/*----------------------------------------------------------------------
 * +++ Spinbuttons.
 *
 * From Apple HIG, part III, section "Controls", "The Stepper Control":
 * there should be 2 pixels of space between the stepper control
 * (AKA IncDecButton, AKA "little arrows") and the text field it modifies.
 */

static Ttk_Padding SpinbuttonMargins = {2,0,2,0};
static void SpinButtonElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SInt32 s;

    ChkErr(GetThemeMetric, kThemeMetricLittleArrowsWidth, &s);
    *widthPtr = s + Ttk_PaddingWidth(SpinbuttonMargins);
    ChkErr(GetThemeMetric, kThemeMetricLittleArrowsHeight, &s);
    *heightPtr = s + Ttk_PaddingHeight(SpinbuttonMargins);
}

static void SpinButtonElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    CGRect bounds = BoxToRect(d, Ttk_PadBox(b, SpinbuttonMargins));
    /* @@@ can't currently distinguish PressedUp (== Pressed) from PressedDown;
     * ignore this bit for now [see #2219588]
     */
    const HIThemeButtonDrawInfo info = {
	.version = 0,
	.state = Ttk_StateTableLookup(ThemeStateTable, state & ~TTK_STATE_PRESSED),
	.kind = kThemeIncDecButton,
	.value = Ttk_StateTableLookup(ButtonValueTable, state),
	.adornment = kThemeAdornmentNone,
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawButton, &bounds, &info, dc.context, HIOrientation, NULL);
    END_DRAWING
}

static Ttk_ElementSpec SpinButtonElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    SpinButtonElementSize,
    SpinButtonElementDraw
};


/*----------------------------------------------------------------------
 * +++ DrawThemeTrack-based elements --
 * Progress bars and scales. (See also: <<NOTE-TRACKS>>)
 */

static Ttk_StateTable ThemeTrackEnableTable[] = {
    { kThemeTrackDisabled, TTK_STATE_DISABLED, 0 },
    { kThemeTrackInactive, TTK_STATE_BACKGROUND, 0 },
    { kThemeTrackActive, 0, 0 }
    /* { kThemeTrackNothingToScroll, ?, ? }, */
};

typedef struct {	/* TrackElement client data */
    ThemeTrackKind	kind;
    SInt32		thicknessMetric;
} TrackElementData;

static TrackElementData ScaleData = {
    kThemeSlider, kThemeMetricHSliderHeight
};


typedef struct {
    Tcl_Obj *fromObj;		/* minimum value */
    Tcl_Obj *toObj;		/* maximum value */
    Tcl_Obj *valueObj;		/* current value */
    Tcl_Obj *orientObj;		/* horizontal / vertical */
} TrackElement;

static Ttk_ElementOptionSpec TrackElementOptions[] = {
    { "-from", TK_OPTION_DOUBLE, Tk_Offset(TrackElement,fromObj) },
    { "-to", TK_OPTION_DOUBLE, Tk_Offset(TrackElement,toObj) },
    { "-value", TK_OPTION_DOUBLE, Tk_Offset(TrackElement,valueObj) },
    { "-orient", TK_OPTION_STRING, Tk_Offset(TrackElement,orientObj) },
    {0,0,0}
};

static void TrackElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    TrackElementData *data = clientData;
    SInt32 size = 24;	/* reasonable default ... */

    ChkErr(GetThemeMetric, data->thicknessMetric, &size);
    *widthPtr = *heightPtr = size;
}

static void TrackElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    TrackElementData *data = clientData;
    TrackElement *elem = elementRecord;
    int orientation = TTK_ORIENT_HORIZONTAL;
    double from = 0, to = 100, value = 0, factor;

    Ttk_GetOrientFromObj(NULL, elem->orientObj, &orientation);
    Tcl_GetDoubleFromObj(NULL, elem->fromObj, &from);
    Tcl_GetDoubleFromObj(NULL, elem->toObj, &to);
    Tcl_GetDoubleFromObj(NULL, elem->valueObj, &value);
    factor = RangeToFactor(to - from);

    HIThemeTrackDrawInfo info = {
	.version = 0,
	.kind = data->kind,
	.bounds = BoxToRect(d, b),
	.min = from * factor,
	.max = to * factor,
	.value = value * factor,
	.attributes = kThemeTrackShowThumb |
		(orientation == TTK_ORIENT_HORIZONTAL ?
		kThemeTrackHorizontal : 0),
	.enableState = Ttk_StateTableLookup(ThemeTrackEnableTable, state),
	.trackInfo.progress.phase = 0,
    };

    if (info.kind == kThemeSlider) {
	info.trackInfo.slider.pressState = state & TTK_STATE_PRESSED ?
		kThemeThumbPressed : 0;
	info.trackInfo.slider.thumbDir = kThemeThumbPlain;
    }


    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawTrack, &info, NULL, dc.context, HIOrientation);
    END_DRAWING
}

static Ttk_ElementSpec TrackElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(TrackElement),
    TrackElementOptions,
    TrackElementSize,
    TrackElementDraw
};

/*
 * Slider element -- <<NOTE-TRACKS>>
 * Has geometry only. The Scale widget adjusts the position of this element,
 * and uses it for hit detection. In the Aqua theme, the slider is actually
 * drawn as part of the trough element.
 *
 * Also buggy: The geometry here is a Wild-Assed-Guess; I can't
 * figure out how to get the Appearance Manager to tell me the
 * slider size.
 */
static void SliderElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *widthPtr = *heightPtr = 24;
}

static Ttk_ElementSpec SliderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    SliderElementSize,
    TtkNullElementDraw
};

/*----------------------------------------------------------------------
 * +++ Progress bar element (new):
 *
 * @@@ NOTE: According to an older revision of the Aqua reference docs,
 * @@@ the 'phase' field is between 0 and 4. Newer revisions say
 * @@@ that it can be any UInt8 value.
 */

typedef struct {
    Tcl_Obj *orientObj;		/* horizontal / vertical */
    Tcl_Obj *valueObj;		/* current value */
    Tcl_Obj *maximumObj;	/* maximum value */
    Tcl_Obj *phaseObj;		/* animation phase */
    Tcl_Obj *modeObj;		/* progress bar mode */
} PbarElement;

static Ttk_ElementOptionSpec PbarElementOptions[] = {
    { "-orient", TK_OPTION_STRING,
	Tk_Offset(PbarElement,orientObj), "horizontal" },
    { "-value", TK_OPTION_DOUBLE,
	Tk_Offset(PbarElement,valueObj), "0" },
    { "-maximum", TK_OPTION_DOUBLE,
	Tk_Offset(PbarElement,maximumObj), "100" },
    { "-phase", TK_OPTION_INT,
	Tk_Offset(PbarElement,phaseObj), "0" },
    { "-mode", TK_OPTION_STRING,
	Tk_Offset(PbarElement,modeObj), "determinate" },
    {0,0,0,0}
};

static void PbarElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SInt32 size = 24;	/* @@@ Check HIG for correct default */

    ChkErr(GetThemeMetric, kThemeMetricLargeProgressBarThickness, &size);
    *widthPtr = *heightPtr = size;
}

static void PbarElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    PbarElement *pbar = elementRecord;
    int orientation = TTK_ORIENT_HORIZONTAL, phase = 0;
    double value = 0, maximum = 100, factor;

    Ttk_GetOrientFromObj(NULL, pbar->orientObj, &orientation);
    Tcl_GetDoubleFromObj(NULL, pbar->valueObj, &value);
    Tcl_GetDoubleFromObj(NULL, pbar->maximumObj, &maximum);
    Tcl_GetIntFromObj(NULL, pbar->phaseObj, &phase);
    factor = RangeToFactor(maximum);

    HIThemeTrackDrawInfo info = {
	.version = 0,
	.kind = (!strcmp("indeterminate", Tcl_GetString(pbar->modeObj)) && value) ?
		kThemeIndeterminateBar : kThemeProgressBar,
	.bounds = BoxToRect(d, b),
	.min = 0,
	.max = maximum * factor,
	.value = value * factor,
	.attributes = kThemeTrackShowThumb |
		(orientation == TTK_ORIENT_HORIZONTAL ?
		kThemeTrackHorizontal : 0),
	.enableState = Ttk_StateTableLookup(ThemeTrackEnableTable, state),
	.trackInfo.progress.phase = phase,
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawTrack, &info, NULL, dc.context, HIOrientation);
    END_DRAWING
}

static Ttk_ElementSpec PbarElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(PbarElement),
    PbarElementOptions,
    PbarElementSize,
    PbarElementDraw
};

/*----------------------------------------------------------------------
 * +++ Separator element.
 *
 * DrawThemeSeparator() guesses the orientation of the line from
 * the width and height of the rectangle, so the same element can
 * can be used for horizontal, vertical, and general separators.
 */

static void SeparatorElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    *widthPtr = *heightPtr = 1;
}

static void SeparatorElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    CGRect bounds = BoxToRect(d, b);
    const HIThemeSeparatorDrawInfo info = {
	.version = 0,
	/* Separator only supports kThemeStateActive, kThemeStateInactive */
	.state = Ttk_StateTableLookup(ThemeStateTable, state & TTK_STATE_BACKGROUND),
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawSeparator, &bounds, &info, dc.context, HIOrientation);
    END_DRAWING
}

static Ttk_ElementSpec SeparatorElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    SeparatorElementSize,
    SeparatorElementDraw
};

/*----------------------------------------------------------------------
 * +++ Size grip element.
 */
static const ThemeGrowDirection sizegripGrowDirection
	= kThemeGrowRight|kThemeGrowDown;

static void SizegripElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    HIThemeGrowBoxDrawInfo info = {
	.version = 0,
	.state = kThemeStateActive,
	.kind = kHIThemeGrowBoxKindNormal,
	.direction = sizegripGrowDirection,
	.size = kHIThemeGrowBoxSizeNormal,
    };
    CGRect bounds = CGRectZero;

    ChkErr(HIThemeGetGrowBoxBounds, &bounds.origin, &info, &bounds);
    *widthPtr = bounds.size.width;
    *heightPtr = bounds.size.height;
}

static void SizegripElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    CGRect bounds = BoxToRect(d, b);
    HIThemeGrowBoxDrawInfo info = {
	.version = 0,
	/* Grow box only supports kThemeStateActive, kThemeStateInactive */
	.state = Ttk_StateTableLookup(ThemeStateTable, state & TTK_STATE_BACKGROUND),
	.kind = kHIThemeGrowBoxKindNormal,
	.direction = sizegripGrowDirection,
	.size = kHIThemeGrowBoxSizeNormal,
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawGrowBox, &bounds.origin, &info, dc.context, HIOrientation);
    END_DRAWING
}

static Ttk_ElementSpec SizegripElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    SizegripElementSize,
    SizegripElementDraw
};

/*----------------------------------------------------------------------
 * +++ Background and fill elements.
 *
 *	This isn't quite right: In Aqua, the correct background for
 *	a control depends on what kind of container it belongs to,
 *	and the type of the top-level window.
 *
 *	Also: patterned backgrounds should be aligned with the coordinate
 *	system of the top-level window.  If we're drawing into an
 *	off-screen graphics port this leads to alignment glitches.
 */

static void FillElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    CGRect bounds = BoxToRect(d, b);
    ThemeBrush brush = (state & TTK_STATE_BACKGROUND)
	    ? kThemeBrushModelessDialogBackgroundInactive
	    : kThemeBrushModelessDialogBackgroundActive;

    BEGIN_DRAWING(d)
    ChkErr(HIThemeSetFill, brush, NULL, dc.context, HIOrientation);
    //QDSetPatternOrigin(PatternOrigin(tkwin, d));
    CGContextFillRect(dc.context, bounds);
    END_DRAWING
}

static void BackgroundElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, unsigned int state)
{
    FillElementDraw(
	clientData, elementRecord, tkwin,
	d, Ttk_WinBox(tkwin), state);
}

static Ttk_ElementSpec FillElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    TtkNullElementSize,
    FillElementDraw
};

static Ttk_ElementSpec BackgroundElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    TtkNullElementSize,
    BackgroundElementDraw
};

/*----------------------------------------------------------------------
 * +++ ToolbarBackground element -- toolbar style for frames.
 *
 *	This is very similar to the normal background element, but uses a
 *	different ThemeBrush in order to get the lighter pinstripe effect
 *	used in toolbars. We use SetThemeBackground() rather than
 *	ApplyThemeBackground() in order to get the right style.
 *
 * <URL: http://developer.apple.com/documentation/Carbon/Reference/
 *	Appearance_Manager/appearance_manager/constant_7.html#/
 *	/apple_ref/doc/uid/TP30000243/C005321>
 *
 */
static void ToolbarBackgroundElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    ThemeBrush brush = kThemeBrushToolbarBackground;
    CGRect bounds = BoxToRect(d, Ttk_WinBox(tkwin));

    BEGIN_DRAWING(d)
    ChkErr(HIThemeSetFill, brush, NULL, dc.context, HIOrientation);
    //QDSetPatternOrigin(PatternOrigin(tkwin, d));
    CGContextFillRect(dc.context, bounds);
    END_DRAWING
}

static Ttk_ElementSpec ToolbarBackgroundElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    TtkNullElementSize,
    ToolbarBackgroundElementDraw
};

/*----------------------------------------------------------------------
 * +++ Treeview header
 *	Redefine the header to use a kThemeListHeaderButton.
 */

#define TTK_TREEVIEW_STATE_SORTARROW	TTK_STATE_USER1
static Ttk_StateTable TreeHeaderValueTable[] = {
    { kThemeButtonOn, TTK_STATE_ALTERNATE},
    { kThemeButtonOn, TTK_STATE_SELECTED},
    { kThemeButtonOff, 0}
};
static Ttk_StateTable TreeHeaderAdornmentTable[] = {
    { kThemeAdornmentHeaderButtonSortUp,
	    TTK_STATE_ALTERNATE|TTK_TREEVIEW_STATE_SORTARROW},
    { kThemeAdornmentDefault,
	    TTK_STATE_SELECTED|TTK_TREEVIEW_STATE_SORTARROW},
    { kThemeAdornmentHeaderButtonNoSortArrow, TTK_STATE_ALTERNATE},
    { kThemeAdornmentHeaderButtonNoSortArrow, TTK_STATE_SELECTED},
    { kThemeAdornmentFocus, TTK_STATE_FOCUS},
    { kThemeAdornmentNone, 0}
};

static void TreeHeaderElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    ThemeButtonParams *params = clientData;
    CGRect bounds = BoxToRect(d, b);
    const HIThemeButtonDrawInfo info = {
	.version = 0,
	.state = Ttk_StateTableLookup(ThemeStateTable, state),
	.kind = params->kind,
	.value = Ttk_StateTableLookup(TreeHeaderValueTable, state),
	.adornment = Ttk_StateTableLookup(TreeHeaderAdornmentTable, state),
    };

    BEGIN_DRAWING(d)
    ChkErr(HIThemeDrawButton, &bounds, &info, dc.context, HIOrientation, NULL);
    END_DRAWING
}

static Ttk_ElementSpec TreeHeaderElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    ButtonElementSizeNoPadding,
    TreeHeaderElementDraw
};

/*
 * Disclosure triangle:
 */
#define TTK_TREEVIEW_STATE_OPEN 	TTK_STATE_USER1
#define TTK_TREEVIEW_STATE_LEAF 	TTK_STATE_USER2
static Ttk_StateTable DisclosureValueTable[] = {
    { kThemeDisclosureDown, TTK_TREEVIEW_STATE_OPEN, 0 },
    { kThemeDisclosureRight, 0, 0 },
};

static void DisclosureElementSize(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    int *widthPtr, int *heightPtr, Ttk_Padding *paddingPtr)
{
    SInt32 s;

    ChkErr(GetThemeMetric, kThemeMetricDisclosureTriangleWidth, &s);
    *widthPtr = s;
    ChkErr(GetThemeMetric, kThemeMetricDisclosureTriangleHeight, &s);
    *heightPtr = s;
}

static void DisclosureElementDraw(
    void *clientData, void *elementRecord, Tk_Window tkwin,
    Drawable d, Ttk_Box b, Ttk_State state)
{
    if (!(state & TTK_TREEVIEW_STATE_LEAF)) {
	CGRect bounds = BoxToRect(d, b);
	const HIThemeButtonDrawInfo info = {
	    .version = 0,
	    .state = Ttk_StateTableLookup(ThemeStateTable, state),
	    .kind = kThemeDisclosureTriangle,
	    .value = Ttk_StateTableLookup(DisclosureValueTable, state),
	    .adornment = kThemeAdornmentDrawIndicatorOnly,
	};

	BEGIN_DRAWING(d)
	ChkErr(HIThemeDrawButton, &bounds, &info, dc.context, HIOrientation, NULL);
	END_DRAWING
    }
}

static Ttk_ElementSpec DisclosureElementSpec = {
    TK_STYLE_VERSION_2,
    sizeof(NullElement),
    TtkNullElementOptions,
    DisclosureElementSize,
    DisclosureElementDraw
};

/*----------------------------------------------------------------------
 * +++ Widget layouts.
 */

TTK_BEGIN_LAYOUT_TABLE(LayoutTable)

TTK_LAYOUT("Toolbar",
    TTK_NODE("Toolbar.background", TTK_FILL_BOTH))

TTK_LAYOUT("TButton",
    TTK_GROUP("Button.button", TTK_FILL_BOTH,
	TTK_GROUP("Button.padding", TTK_FILL_BOTH,
	    TTK_NODE("Button.label", TTK_FILL_BOTH))))

TTK_LAYOUT("TRadiobutton",
    TTK_GROUP("Radiobutton.button", TTK_FILL_BOTH,
	TTK_GROUP("Radiobutton.padding", TTK_FILL_BOTH,
	    TTK_NODE("Radiobutton.label", TTK_PACK_LEFT))))

TTK_LAYOUT("TCheckbutton",
    TTK_GROUP("Checkbutton.button", TTK_FILL_BOTH,
	TTK_GROUP("Checkbutton.padding", TTK_FILL_BOTH,
	    TTK_NODE("Checkbutton.label", TTK_PACK_LEFT))))

TTK_LAYOUT("TMenubutton",
    TTK_GROUP("Menubutton.button", TTK_FILL_BOTH,
	TTK_GROUP("Menubutton.padding", TTK_FILL_BOTH,
	    TTK_NODE("Menubutton.label", TTK_PACK_LEFT))))

TTK_LAYOUT("TCombobox",
    TTK_GROUP("Combobox.button", TTK_PACK_TOP|TTK_FILL_X,
	TTK_GROUP("Combobox.padding", TTK_FILL_BOTH,
	    TTK_NODE("Combobox.textarea", TTK_FILL_X))))

/* Notebook tabs -- no focus ring */
TTK_LAYOUT("Tab",
    TTK_GROUP("Notebook.tab", TTK_FILL_BOTH,
	TTK_GROUP("Notebook.padding", TTK_EXPAND|TTK_FILL_BOTH,
	    TTK_NODE("Notebook.label", TTK_EXPAND|TTK_FILL_BOTH))))

/* Progress bars -- track only */
TTK_LAYOUT("TSpinbox",
    TTK_NODE("Spinbox.spinbutton", TTK_PACK_RIGHT|TTK_STICK_E)
    TTK_GROUP("Spinbox.field", TTK_EXPAND|TTK_FILL_X,
	TTK_NODE("Spinbox.textarea", TTK_EXPAND|TTK_FILL_X)))

TTK_LAYOUT("TProgressbar",
    TTK_NODE("Progressbar.track", TTK_EXPAND|TTK_FILL_BOTH))

/* Tree heading -- no border, fixed height */
TTK_LAYOUT("Heading",
    TTK_NODE("Treeheading.cell", TTK_FILL_X)
    TTK_NODE("Treeheading.image", TTK_PACK_RIGHT)
    TTK_NODE("Treeheading.text", 0))

/* Tree items -- omit focus ring */
TTK_LAYOUT("Item",
    TTK_GROUP("Treeitem.padding", TTK_FILL_BOTH,
	TTK_NODE("Treeitem.indicator", TTK_PACK_LEFT)
	TTK_NODE("Treeitem.image", TTK_PACK_LEFT)
	TTK_NODE("Treeitem.text", TTK_PACK_LEFT)))

TTK_END_LAYOUT_TABLE

/*----------------------------------------------------------------------
 * +++ Initialization.
 */

static int AquaTheme_Init(Tcl_Interp *interp)
{
    Ttk_Theme themePtr = Ttk_CreateTheme(interp, "aqua", NULL);

    if (!themePtr) {
	return TCL_ERROR;
    }

    /*
     * Elements:
     */
    Ttk_RegisterElementSpec(themePtr, "background", &BackgroundElementSpec, 0);
    Ttk_RegisterElementSpec(themePtr, "fill", &FillElementSpec, 0);
    Ttk_RegisterElementSpec(themePtr, "Toolbar.background",
	&ToolbarBackgroundElementSpec, 0);

    Ttk_RegisterElementSpec(themePtr, "Button.button",
	&ButtonElementSpec, &PushButtonParams);
    Ttk_RegisterElementSpec(themePtr, "Checkbutton.button",
	&ButtonElementSpec, &CheckBoxParams);
    Ttk_RegisterElementSpec(themePtr, "Radiobutton.button",
	&ButtonElementSpec, &RadioButtonParams);
    Ttk_RegisterElementSpec(themePtr, "Toolbutton.border",
	&ButtonElementSpec, &BevelButtonParams);
    Ttk_RegisterElementSpec(themePtr, "Menubutton.button",
	&ButtonElementSpec, &PopupButtonParams);
    Ttk_RegisterElementSpec(themePtr, "Spinbox.spinbutton",
    	&SpinButtonElementSpec, 0);
    Ttk_RegisterElementSpec(themePtr, "Combobox.button",
	&ComboboxElementSpec, 0);
    Ttk_RegisterElementSpec(themePtr, "Treeitem.indicator",
	&DisclosureElementSpec, &DisclosureParams);
    Ttk_RegisterElementSpec(themePtr, "Treeheading.cell",
	&TreeHeaderElementSpec, &ListHeaderParams);

    Ttk_RegisterElementSpec(themePtr, "Notebook.tab", &TabElementSpec, 0);
    Ttk_RegisterElementSpec(themePtr, "Notebook.client", &PaneElementSpec, 0);

    Ttk_RegisterElementSpec(themePtr, "Labelframe.border",&GroupElementSpec,0);
    Ttk_RegisterElementSpec(themePtr, "Entry.field",&EntryElementSpec,0);
    Ttk_RegisterElementSpec(themePtr, "Spinbox.field",&EntryElementSpec,0);

    Ttk_RegisterElementSpec(themePtr, "separator",&SeparatorElementSpec,0);
    Ttk_RegisterElementSpec(themePtr, "hseparator",&SeparatorElementSpec,0);
    Ttk_RegisterElementSpec(themePtr, "vseparator",&SeparatorElementSpec,0);

    Ttk_RegisterElementSpec(themePtr, "sizegrip",&SizegripElementSpec,0);

    /*
     * <<NOTE-TRACKS>>
     * The Progressbar widget adjusts the size of the pbar element.
     * In the Aqua theme, the appearance manager computes the bar geometry;
     * we do all the drawing in the ".track" element and leave the .pbar out.
     */
    Ttk_RegisterElementSpec(themePtr,"Scale.trough",
	&TrackElementSpec, &ScaleData);
    Ttk_RegisterElementSpec(themePtr,"Scale.slider",&SliderElementSpec,0);
    Ttk_RegisterElementSpec(themePtr,"Progressbar.track", &PbarElementSpec, 0);

    /*
     * Layouts:
     */
    Ttk_RegisterLayouts(themePtr, LayoutTable);

    Tcl_PkgProvide(interp, "ttk::theme::aqua", TTK_VERSION);
    return TCL_OK;
}

MODULE_SCOPE
int Ttk_MacOSXPlatformInit(Tcl_Interp *interp)
{
    return AquaTheme_Init(interp);
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */

