/*
 * tixDef.h --
 *
 *	This file defines the defaults for all options for all of
 *	the Tix widgets.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000      Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixDef.h,v 1.6 2004/03/28 02:44:56 hobbs Exp $
 */

#ifndef TIX_DEFAULT
#define TIX_DEFAULT

/*
 * The definitions below provide symbolic names for the default colors.
 * NORMAL_BG -		Normal background color.
 * ACTIVE_BG -		Background color when widget is active.
 * SELECT_BG -		Background color for selected text.
 * TROUGH -		Background color for troughs in scales and scrollbars.
 * INDICATOR -		Color for indicator when button is selected.
 * DISABLED -		Foreground color when widget is disabled.
 */

#define BLACK		"Black"
#define WHITE		"White"

#if defined(__WIN32__) || defined(_WIN32)

/*
 * Defaults from Tk
 */

#define CTL_FONT	"{MS Sans Serif} 8"
#define NORMAL_BG	"SystemButtonFace"
#define NORMAL_FG	"SystemButtonText"
#define ACTIVE_BG	NORMAL_BG
#define TEXT_FG		"SystemWindowText"
#define SELECT_BG	"SystemHighlight"
#define SELECT_FG	"SystemHighlightText"
#define TROUGH		"SystemScrollbar"
#define INDICATOR	"SystemWindow"
#define DISABLED	"SystemDisabledText"
#define MENU_BG		"SystemMenu"
#define MENU_FG		"SystemMenuText"
#define HIGHLIGHT	"SystemWindowFrame"

#   include <tixWinDefault.h>
#else
#   if defined(MAC_TCL)
#	include <tixMacDefault.h>
#   else

#define NORMAL_BG	"#d9d9d9"
#define ACTIVE_BG	"#ececec"
#define SELECT_BG	"#c3c3c3"
#define TROUGH		"#c3c3c3"
#define INDICATOR	"#b03060"
#define DISABLED	"#a3a3a3"

#	include <tixUnixDefault.h>
#   endif
#endif

#endif /* TIX_DEFAULT */
