/****************************************************************************
*                                                                           *
* HtmlHelp.h                                                                *
*                                                                           *
* Copyright (c) 1996-1997, Microsoft Corp. All rights reserved.             *
*                                                                           *
****************************************************************************/

#if _MSC_VER > 1000
#pragma once
#endif

#ifndef __HTMLHELP_H__
#define __HTMLHELP_H__

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Defines for Win64
#ifndef _WIN64
#define DWORD_PTR DWORD
#endif

// Commands to pass to HtmlHelp()

#define HH_DISPLAY_TOPIC        0x0000
#define HH_HELP_FINDER          0x0000  // WinHelp equivalent
#define HH_DISPLAY_TOC          0x0001  
#define HH_DISPLAY_INDEX        0x0002  
#define HH_DISPLAY_SEARCH       0x0003  
#define HH_SET_WIN_TYPE         0x0004
#define HH_GET_WIN_TYPE         0x0005
#define HH_GET_WIN_HANDLE       0x0006
#define HH_ENUM_INFO_TYPE       0x0007  // Get Info type name, call repeatedly to enumerate, -1 at end
#define HH_SET_INFO_TYPE        0x0008  // Add Info type to filter.
#define HH_SYNC                 0x0009
#define HH_RESERVED1            0x000A
#define HH_RESERVED2            0x000B
#define HH_RESERVED3            0x000C
#define HH_KEYWORD_LOOKUP       0x000D
#define HH_DISPLAY_TEXT_POPUP   0x000E  // display string resource id or text in a popup window
#define HH_HELP_CONTEXT         0x000F  // display mapped numeric value in dwData
#define HH_TP_HELP_CONTEXTMENU  0x0010  // text popup help, same as WinHelp HELP_CONTEXTMENU
#define HH_TP_HELP_WM_HELP      0x0011  // text popup help, same as WinHelp HELP_WM_HELP
#define HH_CLOSE_ALL            0x0012  // close all windows opened directly or indirectly by the caller
#define HH_ALINK_LOOKUP         0x0013  // ALink version of HH_KEYWORD_LOOKUP
#define HH_GET_LAST_ERROR       0x0014  // not currently implemented // See HHERROR.h
#define HH_ENUM_CATEGORY        0x0015	// Get category name, call repeatedly to enumerate, -1 at end
#define HH_ENUM_CATEGORY_IT     0x0016  // Get category info type members, call repeatedly to enumerate, -1 at end
#define HH_RESET_IT_FILTER      0x0017  // Clear the info type filter of all info types.
#define HH_SET_INCLUSIVE_FILTER 0x0018  // set inclusive filtering method for untyped topics to be included in display
#define HH_SET_EXCLUSIVE_FILTER 0x0019  // set exclusive filtering method for untyped topics to be excluded from display
#define HH_INITIALIZE            0x001C  // Initializes the help system.
#define HH_UNINITIALIZE          0x001D  // Uninitializes the help system.
#define HH_PRETRANSLATEMESSAGE  0x00fd  // Pumps messages. (NULL, NULL, MSG*). 
#define HH_SET_GLOBAL_PROPERTY  0x00fc  // Set a global property. (NULL, NULL, HH_GPROP)

#define HHWIN_PROP_TAB_AUTOHIDESHOW (1 << 0)    // Automatically hide/show tri-pane window
#define HHWIN_PROP_ONTOP            (1 << 1)    // Top-most window
#define HHWIN_PROP_NOTITLEBAR       (1 << 2)    // no title bar
#define HHWIN_PROP_NODEF_STYLES     (1 << 3)    // no default window styles (only HH_WINTYPE.dwStyles)
#define HHWIN_PROP_NODEF_EXSTYLES   (1 << 4)    // no default extended window styles (only HH_WINTYPE.dwExStyles)
#define HHWIN_PROP_TRI_PANE         (1 << 5)    // use a tri-pane window
#define HHWIN_PROP_NOTB_TEXT        (1 << 6)    // no text on toolbar buttons
#define HHWIN_PROP_POST_QUIT        (1 << 7)    // post WM_QUIT message when window closes
#define HHWIN_PROP_AUTO_SYNC        (1 << 8)    // automatically ssync contents and index
#define HHWIN_PROP_TRACKING         (1 << 9)    // send tracking notification messages
#define HHWIN_PROP_TAB_SEARCH       (1 << 10)   // include search tab in navigation pane
#define HHWIN_PROP_TAB_HISTORY      (1 << 11)   // include history tab in navigation pane
#define HHWIN_PROP_TAB_FAVORITES    (1 << 12)   // include favorites tab in navigation pane
#define HHWIN_PROP_CHANGE_TITLE     (1 << 13)   // Put current HTML title in title bar
#define HHWIN_PROP_NAV_ONLY_WIN     (1 << 14)   // Only display the navigation window
#define HHWIN_PROP_NO_TOOLBAR       (1 << 15)   // Don't display a toolbar
#define HHWIN_PROP_MENU             (1 << 16)   // Menu
#define HHWIN_PROP_TAB_ADVSEARCH    (1 << 17)   // Advanced FTS UI.
#define HHWIN_PROP_USER_POS         (1 << 18)   // After initial creation, user controls window size/position
#define HHWIN_PROP_TAB_CUSTOM1      (1 << 19)   // Use custom tab #1
#define HHWIN_PROP_TAB_CUSTOM2      (1 << 20)   // Use custom tab #2
#define HHWIN_PROP_TAB_CUSTOM3      (1 << 21)   // Use custom tab #3
#define HHWIN_PROP_TAB_CUSTOM4      (1 << 22)   // Use custom tab #4
#define HHWIN_PROP_TAB_CUSTOM5      (1 << 23)   // Use custom tab #5
#define HHWIN_PROP_TAB_CUSTOM6      (1 << 24)   // Use custom tab #6
#define HHWIN_PROP_TAB_CUSTOM7      (1 << 25)   // Use custom tab #7
#define HHWIN_PROP_TAB_CUSTOM8      (1 << 26)   // Use custom tab #8
#define HHWIN_PROP_TAB_CUSTOM9      (1 << 27)   // Use custom tab #9
#define HHWIN_TB_MARGIN             (1 << 28)   // the window type has a margin

#define HHWIN_PARAM_PROPERTIES      (1 << 1)    // valid fsWinProperties
#define HHWIN_PARAM_STYLES          (1 << 2)    // valid dwStyles
#define HHWIN_PARAM_EXSTYLES        (1 << 3)    // valid dwExStyles
#define HHWIN_PARAM_RECT            (1 << 4)    // valid rcWindowPos
#define HHWIN_PARAM_NAV_WIDTH       (1 << 5)    // valid iNavWidth
#define HHWIN_PARAM_SHOWSTATE       (1 << 6)    // valid nShowState
#define HHWIN_PARAM_INFOTYPES       (1 << 7)    // valid apInfoTypes
#define HHWIN_PARAM_TB_FLAGS        (1 << 8)    // valid fsToolBarFlags
#define HHWIN_PARAM_EXPANSION       (1 << 9)    // valid fNotExpanded
#define HHWIN_PARAM_TABPOS          (1 << 10)   // valid tabpos
#define HHWIN_PARAM_TABORDER        (1 << 11)   // valid taborder
#define HHWIN_PARAM_HISTORY_COUNT   (1 << 12)   // valid cHistory
#define HHWIN_PARAM_CUR_TAB         (1 << 13)   // valid curNavType

#define HHWIN_BUTTON_EXPAND         (1 << 1)    // Expand/contract button
#define HHWIN_BUTTON_BACK           (1 << 2)    // Back button
#define HHWIN_BUTTON_FORWARD        (1 << 3)    // Forward button
#define HHWIN_BUTTON_STOP           (1 << 4)    // Stop button
#define HHWIN_BUTTON_REFRESH        (1 << 5)    // Refresh button
#define HHWIN_BUTTON_HOME           (1 << 6)    // Home button
#define HHWIN_BUTTON_BROWSE_FWD     (1 << 7)    // not implemented
#define HHWIN_BUTTON_BROWSE_BCK     (1 << 8)    // not implemented
#define HHWIN_BUTTON_NOTES          (1 << 9)    // not implemented
#define HHWIN_BUTTON_CONTENTS       (1 << 10)   // not implemented
#define HHWIN_BUTTON_SYNC           (1 << 11)   // Sync button
#define HHWIN_BUTTON_OPTIONS        (1 << 12)   // Options button
#define HHWIN_BUTTON_PRINT          (1 << 13)   // Print button
#define HHWIN_BUTTON_INDEX          (1 << 14)   // not implemented
#define HHWIN_BUTTON_SEARCH         (1 << 15)   // not implemented
#define HHWIN_BUTTON_HISTORY        (1 << 16)   // not implemented
#define HHWIN_BUTTON_FAVORITES      (1 << 17)   // not implemented
#define HHWIN_BUTTON_JUMP1          (1 << 18)
#define HHWIN_BUTTON_JUMP2          (1 << 19)
#define HHWIN_BUTTON_ZOOM           (1 << 20)
#define HHWIN_BUTTON_TOC_NEXT       (1 << 21)
#define HHWIN_BUTTON_TOC_PREV       (1 << 22)

#define HHWIN_DEF_BUTTONS           \
            (HHWIN_BUTTON_EXPAND |  \
             HHWIN_BUTTON_BACK |    \
             HHWIN_BUTTON_OPTIONS | \
             HHWIN_BUTTON_PRINT)

// Button IDs

#define IDTB_EXPAND             200
#define IDTB_CONTRACT           201
#define IDTB_STOP               202
#define IDTB_REFRESH            203
#define IDTB_BACK               204
#define IDTB_HOME               205
#define IDTB_SYNC               206
#define IDTB_PRINT              207
#define IDTB_OPTIONS            208
#define IDTB_FORWARD            209
#define IDTB_NOTES              210 // not implemented
#define IDTB_BROWSE_FWD         211
#define IDTB_BROWSE_BACK        212
#define IDTB_CONTENTS           213 // not implemented
#define IDTB_INDEX              214 // not implemented
#define IDTB_SEARCH             215 // not implemented
#define IDTB_HISTORY            216 // not implemented
#define IDTB_FAVORITES          217 // not implemented
#define IDTB_JUMP1              218
#define IDTB_JUMP2              219
#define IDTB_CUSTOMIZE          221
#define IDTB_ZOOM               222
#define IDTB_TOC_NEXT           223
#define IDTB_TOC_PREV           224

// Notification codes

#define HHN_FIRST       (0U-860U)
#define HHN_LAST        (0U-879U)

#define HHN_NAVCOMPLETE   (HHN_FIRST-0)
#define HHN_TRACK         (HHN_FIRST-1)
#define HHN_WINDOW_CREATE (HHN_FIRST-2)

typedef struct tagHHN_NOTIFY
{
    NMHDR   hdr;
    PCSTR   pszUrl; // Multi-byte, null-terminated string
} HHN_NOTIFY;

typedef struct tagHH_POPUP
{
    int       cbStruct;      // sizeof this structure
    HINSTANCE hinst;         // instance handle for string resource
    UINT      idString;      // string resource id, or text id if pszFile is specified in HtmlHelp call
    LPCTSTR   pszText;       // used if idString is zero
    POINT     pt;            // top center of popup window
    COLORREF  clrForeground; // use -1 for default
    COLORREF  clrBackground; // use -1 for default
    RECT      rcMargins;     // amount of space between edges of window and text, -1 for each member to ignore
    LPCTSTR   pszFont;       // facename, point size, char set, BOLD ITALIC UNDERLINE
} HH_POPUP;

typedef struct tagHH_AKLINK
{
    int       cbStruct;     // sizeof this structure
    BOOL      fReserved;    // must be FALSE (really!)
    LPCTSTR   pszKeywords;  // semi-colon separated keywords
    LPCTSTR   pszUrl;       // URL to jump to if no keywords found (may be NULL)
    LPCTSTR   pszMsgText;   // Message text to display in MessageBox if pszUrl is NULL and no keyword match
    LPCTSTR   pszMsgTitle;  // Message text to display in MessageBox if pszUrl is NULL and no keyword match
    LPCTSTR   pszWindow;    // Window to display URL in
    BOOL      fIndexOnFail; // Displays index if keyword lookup fails.
} HH_AKLINK;

enum {
    HHWIN_NAVTYPE_TOC,
    HHWIN_NAVTYPE_INDEX,
    HHWIN_NAVTYPE_SEARCH,
    HHWIN_NAVTYPE_FAVORITES,
    HHWIN_NAVTYPE_HISTORY,   // not implemented
    HHWIN_NAVTYPE_AUTHOR,
    HHWIN_NAVTYPE_CUSTOM_FIRST = 11
};

enum {
    IT_INCLUSIVE,
    IT_EXCLUSIVE,
    IT_HIDDEN,
};

typedef struct tagHH_ENUM_IT
{
    int       cbStruct;          // size of this structure
    int       iType;             // the type of the information type ie. Inclusive, Exclusive, or Hidden
    LPCSTR    pszCatName;        // Set to the name of the Category to enumerate the info types in a category; else NULL
    LPCSTR    pszITName;         // volitile pointer to the name of the infotype. Allocated by call. Caller responsible for freeing
    LPCSTR    pszITDescription;  // volitile pointer to the description of the infotype.
} HH_ENUM_IT, *PHH_ENUM_IT;

typedef struct tagHH_ENUM_CAT
{
    int       cbStruct;          // size of this structure
    LPCSTR    pszCatName;        // volitile pointer to the category name
    LPCSTR    pszCatDescription; // volitile pointer to the category description
} HH_ENUM_CAT, *PHH_ENUM_CAT;

typedef struct tagHH_SET_INFOTYPE
{
    int       cbStruct;          // the size of this structure
    LPCSTR    pszCatName;        // the name of the category, if any, the InfoType is a member of.
    LPCSTR    pszInfoTypeName;   // the name of the info type to add to the filter
} HH_SET_INFOTYPE, *PHH_SET_INFOTYPE;

typedef DWORD HH_INFOTYPE;
typedef HH_INFOTYPE* PHH_INFOTYPE;

enum {
    HHWIN_NAVTAB_TOP,
    HHWIN_NAVTAB_LEFT,
    HHWIN_NAVTAB_BOTTOM,
};

#define HH_MAX_TABS 19  // maximum number of tabs

enum {
    HH_TAB_CONTENTS,
    HH_TAB_INDEX,
    HH_TAB_SEARCH,
    HH_TAB_FAVORITES,
    HH_TAB_HISTORY,
    HH_TAB_AUTHOR,

    HH_TAB_CUSTOM_FIRST = 11,
    HH_TAB_CUSTOM_LAST = HH_MAX_TABS
};

#define HH_MAX_TABS_CUSTOM (HH_TAB_CUSTOM_LAST - HH_TAB_CUSTOM_FIRST + 1)

// HH_DISPLAY_SEARCH Command Related Structures and Constants

#define HH_FTS_DEFAULT_PROXIMITY (-1)

typedef struct tagHH_FTS_QUERY
{
    int cbStruct;            // Sizeof structure in bytes.
    BOOL fUniCodeStrings;    // TRUE if all strings are unicode.
    LPCTSTR pszSearchQuery;  // String containing the search query.
    LONG iProximity;         // Word proximity.
    BOOL fStemmedSearch;     // TRUE for StemmedSearch only.
    BOOL fTitleOnly;         // TRUE for Title search only.
    BOOL fExecute;           // TRUE to initiate the search.
    LPCTSTR pszWindow;       // Window to display in
} HH_FTS_QUERY;

// HH_WINTYPE Structure

typedef struct tagHH_WINTYPE {
    int     cbStruct;        // IN: size of this structure including all Information Types
    BOOL    fUniCodeStrings; // IN/OUT: TRUE if all strings are in UNICODE
    LPCTSTR pszType;         // IN/OUT: Name of a type of window
    DWORD   fsValidMembers;  // IN: Bit flag of valid members (HHWIN_PARAM_)
    DWORD   fsWinProperties; // IN/OUT: Properties/attributes of the window (HHWIN_)

    LPCTSTR pszCaption;      // IN/OUT: Window title
    DWORD   dwStyles;        // IN/OUT: Window styles
    DWORD   dwExStyles;      // IN/OUT: Extended Window styles
    RECT    rcWindowPos;     // IN: Starting position, OUT: current position
    int     nShowState;      // IN: show state (e.g., SW_SHOW)

    HWND  hwndHelp;          // OUT: window handle
    HWND  hwndCaller;        // OUT: who called this window

    HH_INFOTYPE* paInfoTypes;  // IN: Pointer to an array of Information Types

    // The following members are only valid if HHWIN_PROP_TRI_PANE is set

    HWND  hwndToolBar;      // OUT: toolbar window in tri-pane window
    HWND  hwndNavigation;   // OUT: navigation window in tri-pane window
    HWND  hwndHTML;         // OUT: window displaying HTML in tri-pane window
    int   iNavWidth;        // IN/OUT: width of navigation window
    RECT  rcHTML;           // OUT: HTML window coordinates

    LPCTSTR pszToc;         // IN: Location of the table of contents file
    LPCTSTR pszIndex;       // IN: Location of the index file
    LPCTSTR pszFile;        // IN: Default location of the html file
    LPCTSTR pszHome;        // IN/OUT: html file to display when Home button is clicked
    DWORD   fsToolBarFlags; // IN: flags controling the appearance of the toolbar
    BOOL    fNotExpanded;   // IN: TRUE/FALSE to contract or expand, OUT: current state
    int     curNavType;     // IN/OUT: UI to display in the navigational pane
    int     tabpos;         // IN/OUT: HHWIN_NAVTAB_TOP, HHWIN_NAVTAB_LEFT, or HHWIN_NAVTAB_BOTTOM
    int     idNotify;       // IN: ID to use for WM_NOTIFY messages
    BYTE    tabOrder[HH_MAX_TABS + 1];    // IN/OUT: tab order: Contents, Index, Search, History, Favorites, Reserved 1-5, Custom tabs
    int     cHistory;       // IN/OUT: number of history items to keep (default is 30)
    LPCTSTR pszJump1;       // Text for HHWIN_BUTTON_JUMP1
    LPCTSTR pszJump2;       // Text for HHWIN_BUTTON_JUMP2
    LPCTSTR pszUrlJump1;    // URL for HHWIN_BUTTON_JUMP1
    LPCTSTR pszUrlJump2;    // URL for HHWIN_BUTTON_JUMP2
    RECT    rcMinSize;      // Minimum size for window (ignored in version 1)
    int     cbInfoTypes;    // size of paInfoTypes;
    LPCTSTR pszCustomTabs;  // multiple zero-terminated strings
} HH_WINTYPE, *PHH_WINTYPE;

enum {
    HHACT_TAB_CONTENTS,
    HHACT_TAB_INDEX,
    HHACT_TAB_SEARCH,
    HHACT_TAB_HISTORY,
    HHACT_TAB_FAVORITES,

    HHACT_EXPAND,
    HHACT_CONTRACT,
    HHACT_BACK,
    HHACT_FORWARD,
    HHACT_STOP,
    HHACT_REFRESH,
    HHACT_HOME,
    HHACT_SYNC,
    HHACT_OPTIONS,
    HHACT_PRINT,
    HHACT_HIGHLIGHT,
    HHACT_CUSTOMIZE,
    HHACT_JUMP1,
    HHACT_JUMP2,
    HHACT_ZOOM,
    HHACT_TOC_NEXT,
    HHACT_TOC_PREV,
    HHACT_NOTES,

    HHACT_LAST_ENUM,
};

typedef struct tagHHNTRACK
{
    NMHDR   hdr;
    PCSTR   pszCurUrl;      // Multi-byte, null-terminated string
    int     idAction;       // HHACT_ value
    HH_WINTYPE* phhWinType; // Current window type structure
} HHNTRACK;

HWND
WINAPI
HtmlHelpA(
    HWND hwndCaller,
    LPCSTR pszFile,
    UINT uCommand,
    DWORD_PTR dwData
    );

HWND
WINAPI
HtmlHelpW(
    HWND hwndCaller,
    LPCWSTR pszFile,
    UINT uCommand,
    DWORD_PTR dwData
    );
#ifdef UNICODE
#define HtmlHelp  HtmlHelpW
#else
#define HtmlHelp  HtmlHelpA
#endif // !UNICODE

// Use the following for GetProcAddress to load from hhctrl.ocx

#define ATOM_HTMLHELP_API_ANSI    (LPTSTR)((DWORD)((WORD)(14)))
#define ATOM_HTMLHELP_API_UNICODE (LPTSTR)((DWORD)((WORD)(15)))

///////////////////////////////////////////////////////////////////////////////
//
// Global Control Properties. 
//
typedef enum tagHH_GPROPID
{
    HH_GPROPID_SINGLETHREAD=1,      // VARIANT_BOOL: True for single thread
    HH_GPROPID_TOOLBAR_MARGIN=2,    // long: Provides a left/right margin around the toolbar.
    HH_GPROPID_UI_LANGUAGE=3,       // long: LangId of the UI.
    HH_GPROPID_CURRENT_SUBSET=4,    // BSTR: Current subset.
    HH_GPROPID_CONTENT_LANGUAGE=5   // long: LandId for desired content.
} HH_GPROPID;

///////////////////////////////////////////////////////////////////////////////
//
// Global Property structure
//
#ifdef __oaidl_h__

#pragma pack(push, 8)

typedef struct tagHH_GLOBAL_PROPERTY
{
    HH_GPROPID  id;
    VARIANT     var;
} HH_GLOBAL_PROPERTY ;

#pragma pack(pop)
#endif

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __HTMLHELP_H__
