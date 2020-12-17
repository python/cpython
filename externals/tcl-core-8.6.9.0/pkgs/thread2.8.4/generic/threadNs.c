/*
 * threadNs.c --
 *
 * Adds interface for loading the extension into the NaviServer/AOLserver.
 *
 * Copyright (c) 2002 by Zoran Vasiljevic.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ---------------------------------------------------------------------------
 */

#ifdef NS_AOLSERVER
#include <ns.h>
#include "tclThreadInt.h"

int Ns_ModuleVersion = 1;

/*
 *----------------------------------------------------------------------------
 *
 * NsThread_Init --
 *
 *    Loads the package for the first time, i.e. in the startup thread.
 *
 * Results:
 *    Standard Tcl result
 *
 * Side effects:
 *    Package initialized. Tcl commands created.
 *
 *----------------------------------------------------------------------------
 */

static int
NsThread_Init (Tcl_Interp *interp, void *cd)
{
    NsThreadInterpData *md = (NsThreadInterpData*)cd;
    int ret = Thread_Init(interp);

    if (ret != TCL_OK) {
        Ns_Log(Warning, "can't load module %s: %s", md->modname,
                Tcl_GetString(Tcl_GetObjResult(interp)));
        return TCL_ERROR;
    }
    Tcl_SetAssocData(interp, "thread:nsd", NULL, (ClientData)md);

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------------
 *
 * Ns_ModuleInit --
 *
 *    Called by the NaviServer/AOLserver when loading shared object file.
 *
 * Results:
 *    Standard NaviServer/AOLserver result
 *
 * Side effects:
 *    Many. Depends on the package.
 *
 *----------------------------------------------------------------------------
 */

int
Ns_ModuleInit(char *srv, char *mod)
{
    NsThreadInterpData *md = NULL;

    md = (NsThreadInterpData*)ns_malloc(sizeof(NsThreadInterpData));
    md->modname = strcpy(ns_malloc(strlen(mod)+1), mod);
    md->server  = strcpy(ns_malloc(strlen(srv)+1), srv);

    return Ns_TclRegisterTrace(srv, NsThread_Init, (void*)md, NS_TCL_TRACE_CREATE);
}

#endif /* NS_AOLSERVER */

/* EOF $RCSfile: aolstub.cpp,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */
