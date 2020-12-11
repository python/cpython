
/*	$Id: tixPort.h,v 1.6 2005/03/25 20:15:53 hobbs Exp $	*/

/*
 * tixPort.h --
 *
 *	This header file handles porting issues that occur because of
 *	differences between systems.  It reads in platform specific
 *	portability files.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _TIX_PORT_H_
#define _TIX_PORT_H_

#ifndef _TCL
#include "tcl.h"
#endif

#ifndef _TK
#include "tk.h"
#endif

#if (!defined(__WIN32__)) && (!defined(_WIN32)) && (!defined(MAC_TCL))
    /*
     * The Tcl/Tk porting stuff is needed only in Unix.
     */
#if !defined(_TCLPORT) && !defined(_TKPORT)
#  ifdef _TKINT
#    include "tkPort.h"
#  else
#    include "tclPort.h"
#  endif
#endif
#endif

#ifndef _TIX_H_
#include <tix.h>
#endif

#if defined(__WIN32__) || defined(_WIN32)
#   include "tixWinPort.h"
#else
#   if defined(MAC_TCL)
#	include "tixMacPort.h"
#   else
#   if defined(MAC_OSX_TK)
#	include <X11/X.h>
#	define Cursor XCursor
#	define Region XRegion
#	include "../unix/tixUnixPort.h"
#   else
#	include "../unix/tixUnixPort.h"
#   endif
#   endif
#endif

EXTERN Tcl_HashTable *	TixGetHashTable _ANSI_ARGS_((Tcl_Interp * interp,
			    char * name, Tcl_InterpDeleteProc *deleteProc,
                            int keyType));

/*
 * Some pre-Tk8.0 style font handling. Should be updated to new Tk
 * font functions soon.
 */

typedef Tk_Font TixFont;
#define TixFontId(font) Tk_FontId(font)

EXTERN void		TixComputeTextGeometry _ANSI_ARGS_((
			    TixFont fontStructPtr, CONST84 char *string,
			    int numChars, int wrapLength, int *widthPtr,
			    int *heightPtr));
EXTERN void		TixDisplayText _ANSI_ARGS_((Display *display,
			    Drawable drawable, TixFont font,
			    CONST84 char *string, int numChars, int x, int y,
			    int length, Tk_Justify justify, int underline,
			    GC gc));

#define TixFreeFont Tk_FreeFont
#define TixNameOfFont Tk_NameOfFont
#define TixGetFont Tk_GetFont

/*
 * Tcl 8.4.a2 defines this macro that causes Tix compiled with
 * Tcl 8.4 to break in Tcl 8.3
 */

#if defined(Tcl_InitHashTable) && defined(USE_TCL_STUBS)
#undef Tcl_InitHashTable
#define Tcl_InitHashTable \
    (tclStubsPtr->tcl_InitHashTable)
#endif

#endif /* _TIX_PORT_H_ */
