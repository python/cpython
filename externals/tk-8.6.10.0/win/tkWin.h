/*
 * tkWin.h --
 *
 *	Declarations of public types and interfaces that are only
 *	available under Windows.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TKWIN
#define _TKWIN

/*
 * We must specify the lower version we intend to support. In particular
 * the SystemParametersInfo API doesn't like to receive structures that
 * are larger than it expects which affects the font assignments.
 *
 * WINVER = 0x0500 means Windows 2000 and above
 */

#ifndef WINVER
#define WINVER 0x0500
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#ifndef _TK
#include <tk.h>
#endif

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN

/*
 * The following messages are used to communicate between a Tk toplevel
 * and its container window. A Tk container may not be able to provide
 * service to all of the following requests at the moment. But an embedded
 * Tk window will send out these requests to support external Tk container
 * application.
 */

#define TK_CLAIMFOCUS	    (WM_USER)	    /* an embedded window requests to focus */
#define TK_GEOMETRYREQ	    (WM_USER+1)	    /* an embedded window requests to change size */
#define TK_ATTACHWINDOW	    (WM_USER+2)	    /* an embedded window requests to attach */
#define TK_DETACHWINDOW	    (WM_USER+3)	    /* an embedded window requests to detach */
#define TK_MOVEWINDOW	    (WM_USER+4)	    /* an embedded window requests to move */
#define TK_RAISEWINDOW	    (WM_USER+5)	    /* an embedded window requests to raise */
#define TK_ICONIFY	    (WM_USER+6)	    /* an embedded window requests to iconify */
#define TK_DEICONIFY	    (WM_USER+7)	    /* an embedded window requests to deiconify */
#define TK_WITHDRAW	    (WM_USER+8)	    /* an embedded window requests to withdraw */
#define TK_GETFRAMEWID	    (WM_USER+9)	    /* an embedded window requests a frame window id */
#define TK_OVERRIDEREDIRECT (WM_USER+10)    /* an embedded window requests to overrideredirect */
#define TK_SETMENU	    (WM_USER+11)    /* an embedded window requests to setup menu */
#define TK_STATE	    (WM_USER+12)    /* an embedded window sets/gets state */
#define TK_INFO		    (WM_USER+13)    /* an embedded window requests a container's info */

/*
 * The following are sub-messages (wParam) for TK_INFO.  An embedded window may
 * send a TK_INFO message with one of the sub-messages to query a container
 * for verification and availability
 */
#define TK_CONTAINER_VERIFY	    0x01
#define TK_CONTAINER_ISAVAILABLE    0x02


/*
 *--------------------------------------------------------------
 *
 * Exported procedures defined for the Windows platform only.
 *
 *--------------------------------------------------------------
 */

#include "tkPlatDecls.h"

#endif /* _TKWIN */
