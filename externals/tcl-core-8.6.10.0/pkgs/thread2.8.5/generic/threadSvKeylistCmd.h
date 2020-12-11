/*
 * threadSvKeylistCmd.h --
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ---------------------------------------------------------------------------
 */

#ifndef _KEYLISTCMDS_H_
#define _KEYLISTCMDS_H_

#include "tclThreadInt.h"

MODULE_SCOPE void Sv_RegisterKeylistCommands(void);
MODULE_SCOPE void TclX_KeyedListInit(Tcl_Interp *interp);

#endif /* _KEYLISTCMDS_H_ */

/* EOF $RCSfile: threadSvKeylistCmd.h,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */

