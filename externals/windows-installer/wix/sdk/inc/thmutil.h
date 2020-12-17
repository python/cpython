#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#define ReleaseTheme(p) if (p) { ThemeFree(p); p = NULL; }

typedef enum THEME_CONTROL_DATA
{
    THEME_CONTROL_DATA_HOVER = 1,
} THEME_CONTROL_DATA;

typedef enum THEME_CONTROL_TYPE
{
    THEME_CONTROL_TYPE_UNKNOWN,
    THEME_CONTROL_TYPE_BILLBOARD,
    THEME_CONTROL_TYPE_BUTTON,
    THEME_CONTROL_TYPE_CHECKBOX,
    THEME_CONTROL_TYPE_EDITBOX,
    THEME_CONTROL_TYPE_HYPERLINK,
    THEME_CONTROL_TYPE_HYPERTEXT,
    THEME_CONTROL_TYPE_IMAGE,
    THEME_CONTROL_TYPE_PROGRESSBAR,
    THEME_CONTROL_TYPE_RICHEDIT,
    THEME_CONTROL_TYPE_STATIC,
    THEME_CONTROL_TYPE_TEXT,
    THEME_CONTROL_TYPE_LISTVIEW,
    THEME_CONTROL_TYPE_TREEVIEW,
    THEME_CONTROL_TYPE_TAB,
    THEME_CONTROL_TYPE_COMBOBOX,
} THEME_CONTROL_TYPE;


struct THEME_BILLBOARD
{
    HBITMAP hImage;
    LPWSTR sczUrl;
};


struct THEME_COLUMN
{
    LPWSTR pszName;
    UINT uStringId;
    int nBaseWidth;
    int nWidth;
    BOOL fExpands;
};


struct THEME_TAB
{
    LPWSTR pszName;
    UINT uStringId;
};

// THEME_ASSIGN_CONTROL_ID - Used to apply a specific id to a named control (usually
//                           to set the WM_COMMAND).
struct THEME_ASSIGN_CONTROL_ID
{
    WORD wId;       // id to apply to control
    LPCWSTR wzName; // name of control to match
};

const DWORD THEME_FIRST_ASSIGN_CONTROL_ID = 1024; // Recommended first control id to be assigned.

struct THEME_CONTROL
{
    THEME_CONTROL_TYPE type;

    WORD wId;
    WORD wPageId;

    LPWSTR sczName; // optional name for control, only used to apply control id.
    LPWSTR sczText;
    int nX;
    int nY;
    int nHeight;
    int nWidth;
    int nSourceX;
    int nSourceY;
    UINT uStringId;

    HBITMAP hImage;

    // Don't free these; it's just a handle to the central image lists stored in THEME. The handle is freed once, there.
    HIMAGELIST rghImageList[4];

    DWORD dwStyle;
    DWORD dwExtendedStyle;
    DWORD dwInternalStyle;

    DWORD dwFontId;
    DWORD dwFontHoverId;
    DWORD dwFontSelectedId;

    // Used by billboard controls
    THEME_BILLBOARD* ptbBillboards;
    DWORD cBillboards;
    WORD wBillboardInterval;
    WORD wBillboardUrls;
    BOOL fBillboardLoops;

    // Used by listview controls
    THEME_COLUMN *ptcColumns;
    DWORD cColumns;

    // Used by tab controls
    THEME_TAB *pttTabs;
    DWORD cTabs;

    // state variables that should be ignored
    HWND hWnd;
    DWORD dwData; // type specific data
};


struct THEME_IMAGELIST
{
    LPWSTR sczName;

    HIMAGELIST hImageList;
};

struct THEME_PAGE
{
    WORD wId;
    LPWSTR sczName;

    DWORD cControlIndices;
    DWORD* rgdwControlIndices;
};

struct THEME_FONT
{
    HFONT hFont;
    COLORREF crForeground;
    HBRUSH hForeground;
    COLORREF crBackground;
    HBRUSH hBackground;
};


struct THEME
{
    WORD wId;

    BOOL fAutoResize;

    DWORD dwStyle;
    DWORD dwFontId;
    HANDLE hIcon;
    LPWSTR sczCaption;
    int nHeight;
    int nMinimumHeight;
    int nWidth;
    int nMinimumWidth;
    int nSourceX;
    int nSourceY;
    UINT uStringId;

    HBITMAP hImage;

    DWORD cFonts;
    THEME_FONT* rgFonts;

    DWORD cPages;
    THEME_PAGE* rgPages;

    DWORD cImageLists;
    THEME_IMAGELIST* rgImageLists;

    DWORD cControls;
    THEME_CONTROL* rgControls;

    // state variables that should be ignored
    HWND hwndParent; // parent for loaded controls
    HWND hwndHover; // current hwnd hovered over
};


/********************************************************************
 ThemeInitialize - initialized theme management.

*******************************************************************/
DAPI_(HRESULT) ThemeInitialize(
    __in_opt HMODULE hModule
    );

/********************************************************************
 ThemeUninitialize - unitialize theme management.

*******************************************************************/
DAPI_(void) ThemeUninitialize();

/********************************************************************
 ThemeLoadFromFile - loads a theme from a loose file.

 *******************************************************************/
DAPI_(HRESULT) ThemeLoadFromFile(
    __in_z LPCWSTR wzThemeFile,
    __out THEME** ppTheme
    );

/********************************************************************
 ThemeLoadFromResource - loads a theme from a module's data resource.

 NOTE: The resource data must be UTF-8 encoded.
*******************************************************************/
DAPI_(HRESULT) ThemeLoadFromResource(
    __in_opt HMODULE hModule,
    __in_z LPCSTR szResource,
    __out THEME** ppTheme
    );

/********************************************************************
 ThemeFree - frees any memory associated with a theme.

*******************************************************************/
DAPI_(void) ThemeFree(
    __in THEME* pTheme
    );

/********************************************************************
 ThemeLoadControls - creates the windows for all the theme controls.

*******************************************************************/
DAPI_(HRESULT) ThemeLoadControls(
    __in THEME* pTheme,
    __in HWND hwndParent,
    __in_ecount_opt(cAssignControlIds) const THEME_ASSIGN_CONTROL_ID* rgAssignControlIds,
    __in DWORD cAssignControlIds
    );

/********************************************************************
 ThemeUnloadControls - resets all the theme control windows so the theme
                       controls can be reloaded.

*******************************************************************/
DAPI_(void) ThemeUnloadControls(
    __in THEME* pTheme
    );

/********************************************************************
 ThemeLocalize - Localizes all of the strings in the theme.

*******************************************************************/
DAPI_(HRESULT) ThemeLocalize(
    __in THEME *pTheme,
    __in const WIX_LOCALIZATION *pLocStringSet
    );

DAPI_(HRESULT) ThemeLoadStrings(
    __in THEME* pTheme,
    __in HMODULE hResModule
    );

/********************************************************************
 ThemeLoadRichEditFromFile - Attach a richedit control to a RTF file.

 *******************************************************************/
DAPI_(HRESULT) ThemeLoadRichEditFromFile(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in_z LPCWSTR wzFileName,
    __in HMODULE hModule
    );

/********************************************************************
 ThemeLoadRichEditFromResource - Attach a richedit control to resource data.

 *******************************************************************/
DAPI_(HRESULT) ThemeLoadRichEditFromResource(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in_z LPCSTR szResourceName,
    __in HMODULE hModule
    );

/********************************************************************
 ThemeLoadRichEditFromResourceToHWnd - Attach a richedit control (by 
                                       HWND) to resource data.

 *******************************************************************/
DAPI_(HRESULT) ThemeLoadRichEditFromResourceToHWnd(
    __in HWND hWnd,
    __in_z LPCSTR szResourceName,
    __in HMODULE hModule
    );

/********************************************************************
 ThemeHandleKeyboardMessage - will translate the message using the active
                             accelerator table.

*******************************************************************/
DAPI_(BOOL) ThemeHandleKeyboardMessage(
    __in_opt THEME* pTheme,
    __in HWND hWnd,
    __in MSG* pMsg
    );

/********************************************************************
 ThemeDefWindowProc - replacement for DefWindowProc() when using theme.

*******************************************************************/
LRESULT CALLBACK ThemeDefWindowProc(
    __in_opt THEME* pTheme,
    __in HWND hWnd,
    __in UINT uMsg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

/********************************************************************
 ThemeGetPageIds - gets the page ids for the theme via page names.

*******************************************************************/
DAPI_(void) ThemeGetPageIds(
    __in const THEME* pTheme,
    __in_ecount(cGetPages) LPCWSTR* rgwzFindNames,
    __in_ecount(cGetPages) DWORD* rgdwPageIds,
    __in DWORD cGetPages
    );

/********************************************************************
 ThemeGetPage - gets a theme page by id.

 *******************************************************************/
DAPI_(THEME_PAGE*) ThemeGetPage(
    __in const THEME* pTheme,
    __in DWORD dwPage
    );

/********************************************************************
 ThemeShowPage - shows or hides all of the controls in the page at one time.

 *******************************************************************/
DAPI_(void) ThemeShowPage(
    __in THEME* pTheme,
    __in DWORD dwPage,
    __in int nCmdShow
    );

/********************************************************************
 ThemeControlExists - check if a control with the specified id exists.

 *******************************************************************/
DAPI_(BOOL) ThemeControlExists(
    __in THEME* pTheme,
    __in DWORD dwControl
    );

/********************************************************************
 ThemeControlEnable - enables/disables a control.

 *******************************************************************/
DAPI_(void) ThemeControlEnable(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in BOOL fEnable
    );

/********************************************************************
 ThemeControlEnabled - returns whether a control is enabled/disabled.

 *******************************************************************/
DAPI_(BOOL) ThemeControlEnabled(
    __in THEME* pTheme,
    __in DWORD dwControl
    );

/********************************************************************
 ThemeControlElevates - sets/removes the shield icon on a control.

 *******************************************************************/
DAPI_(void) ThemeControlElevates(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in BOOL fElevates
    );

/********************************************************************
 ThemeShowControl - shows/hides a control.

 *******************************************************************/
DAPI_(void) ThemeShowControl(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in int nCmdShow
    );

/********************************************************************
 ThemeControlVisible - returns whether a control is visible.

 *******************************************************************/
DAPI_(BOOL) ThemeControlVisible(
    __in THEME* pTheme,
    __in DWORD dwControl
    );

DAPI_(BOOL) ThemePostControlMessage(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

DAPI_(LRESULT) ThemeSendControlMessage(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in UINT Msg,
    __in WPARAM wParam,
    __in LPARAM lParam
    );

/********************************************************************
 ThemeDrawBackground - draws the theme background.

*******************************************************************/
DAPI_(HRESULT) ThemeDrawBackground(
    __in THEME* pTheme,
    __in PAINTSTRUCT* pps
    );

/********************************************************************
 ThemeDrawControl - draw an owner drawn control.

*******************************************************************/
DAPI_(HRESULT) ThemeDrawControl(
    __in THEME* pTheme,
    __in DRAWITEMSTRUCT* pdis
    );

/********************************************************************
 ThemeHoverControl - mark a control as hover.

*******************************************************************/
DAPI_(BOOL) ThemeHoverControl(
    __in THEME* pTheme,
    __in HWND hwndParent,
    __in HWND hwndControl
    );

/********************************************************************
 ThemeIsControlChecked - gets whether a control is checked. Only
                         really useful for checkbox controls.

*******************************************************************/
DAPI_(BOOL) ThemeIsControlChecked(
    __in THEME* pTheme,
    __in DWORD dwControl
    );

/********************************************************************
 ThemeSetControlColor - sets the color of text for a control.

*******************************************************************/
DAPI_(BOOL) ThemeSetControlColor(
    __in THEME* pTheme,
    __in HDC hdc,
    __in HWND hWnd,
    __out HBRUSH* phBackgroundBrush
    );

/********************************************************************
 ThemeStartBillboard - starts a billboard control changing images according
                       to their interval.

 NOTE: iImage specifies the image to start on. If iImage is
       greater than the number of images, the last image shown
       will be the start image.
*******************************************************************/
DAPI_(HRESULT) ThemeStartBillboard(
    __in const THEME* pTheme,
    __in DWORD dwControl,
    __in WORD iImage
    );

/********************************************************************
 ThemeStopBillboard - stops a billboard control from changing images.

*******************************************************************/
DAPI_(HRESULT) ThemeStopBillboard(
    __in const THEME* pTheme,
    __in DWORD dwControl
    );

/********************************************************************
 ThemeSetProgressControl - sets the current percentage complete in a
                           progress bar control.

*******************************************************************/
DAPI_(HRESULT) ThemeSetProgressControl(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in DWORD dwProgressPercentage
    );

/********************************************************************
 ThemeSetProgressControlColor - sets the current color of a
                                progress bar control.

*******************************************************************/
DAPI_(HRESULT) ThemeSetProgressControlColor(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in DWORD dwColorIndex
    );

/********************************************************************
 ThemeSetTextControl - sets the text of a control.

*******************************************************************/
DAPI_(HRESULT) ThemeSetTextControl(
    __in THEME* pTheme,
    __in DWORD dwControl,
    __in_z LPCWSTR wzText
    );

/********************************************************************
 ThemeGetTextControl - gets the text of a control.

*******************************************************************/
DAPI_(HRESULT) ThemeGetTextControl(
    __in const THEME* pTheme,
    __in DWORD dwControl,
    __out LPWSTR* psczText
    );

/********************************************************************
 ThemeUpdateCaption - updates the caption in the theme.

*******************************************************************/
DAPI_(HRESULT) ThemeUpdateCaption(
    __in THEME* pTheme,
    __in_z LPCWSTR wzCaption
    );

/********************************************************************
 ThemeSetFocus - set the focus to the control supplied or the next 
                 enabled control if it is disabled.

*******************************************************************/
DAPI_(void) ThemeSetFocus(
    __in THEME* pTheme,
    __in DWORD dwControl
    );

#ifdef __cplusplus
}
#endif

