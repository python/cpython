
/*	$Id: tixUnixWm.c,v 1.2 2005/03/25 20:15:53 hobbs Exp $	*/

/*
 * tixUnixWm.c --
 *
 *	Implement the Windows specific function calls for window management.
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include "tixUnixInt.h"

int
TixpSetWindowParent(interp, tkwin, newParent, parentId)
    Tcl_Interp * interp;
    Tk_Window tkwin;
    Tk_Window newParent;
    int parentId;
{
    return TCL_OK;
}

#ifdef MAC_OSX_TK
#include "tkInt.h"
/*
 *----------------------------------------------------------------------
 *
 * XLowerWindow --
 *
 *	Change the stacking order of a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Changes the stacking order of the specified window.
 *
 *----------------------------------------------------------------------
 */

int 
XLowerWindow(
    Display* display,		/* Display. */
    Window window)		/* Window. */
{
    TkWindow *winPtr = *((TkWindow **) window);
    
    display->request++;
    if (Tk_IsTopLevel(winPtr) && !Tk_IsEmbedded(winPtr)) {
	TkWmRestackToplevel(winPtr, Below, NULL);
    } else {
    	/* TODO: this should generate damage */
    }
    return 0;
}
#endif
