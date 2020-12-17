/*
 * tclXkeylist.h --
 *
 * Extended Tcl keyed list commands and interfaces.
 *-----------------------------------------------------------------------------
 * Copyright 1991-1999 Karl Lehenbauer and Mark Diekhans.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies.  Karl Lehenbauer and
 * Mark Diekhans make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *-----------------------------------------------------------------------------
 */

#ifndef _KEYLIST_H_
#define _KEYLIST_H_

#include "tclThreadInt.h"

/*
 * Keyed list object interface commands
 */

MODULE_SCOPE Tcl_Obj* TclX_NewKeyedListObj();

MODULE_SCOPE void TclX_KeyedListInit(Tcl_Interp*);
MODULE_SCOPE int  TclX_KeyedListGet(Tcl_Interp*, Tcl_Obj*, const char*, Tcl_Obj**);
MODULE_SCOPE int  TclX_KeyedListSet(Tcl_Interp*, Tcl_Obj*, const char*, Tcl_Obj*);
MODULE_SCOPE int  TclX_KeyedListDelete(Tcl_Interp*, Tcl_Obj*, const char*);
MODULE_SCOPE int  TclX_KeyedListGetKeys(Tcl_Interp*, Tcl_Obj*, const char*, Tcl_Obj**);

/*
 * Exported for usage in Sv_DuplicateObj. This is slightly
 * modified version of the DupKeyedListInternalRep() function.
 * It does a proper deep-copy of the keyed list object.
 */

MODULE_SCOPE void DupKeyedListInternalRepShared(Tcl_Obj*, Tcl_Obj*);

#endif /* _KEYLIST_H_ */

/* EOF $RCSfile: tclXkeylist.h,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */

