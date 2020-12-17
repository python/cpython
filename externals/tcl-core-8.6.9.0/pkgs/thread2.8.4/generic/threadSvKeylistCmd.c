/*
 * threadSvKeylist.c --
 *
 * This file implements keyed-list commands as part of the thread
 * shared variable implementation.
 *
 * Keyed list implementation is borrowed from Mark Diekhans and
 * Karl Lehenbauer "TclX" (extended Tcl) extension. Please look
 * into the keylist.c file for more information.
 *
 * See the file "license.txt" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 * ---------------------------------------------------------------------------
 */

#include "threadSvCmd.h"
#include "threadSvKeylistCmd.h"
#include "tclXkeylist.h"

/*
 * This is defined in keylist.c. We need it here
 * to be able to plug-in our custom keyed-list
 * object duplicator which produces proper deep
 * copies of the keyed-list objects. The standard
 * one produces shallow copies which are not good
 * for usage in the thread shared variables code.
 */

extern Tcl_ObjType keyedListType;

/*
 * Wrapped keyed-list commands
 */

static Tcl_ObjCmdProc SvKeylsetObjCmd;
static Tcl_ObjCmdProc SvKeylgetObjCmd;
static Tcl_ObjCmdProc SvKeyldelObjCmd;
static Tcl_ObjCmdProc SvKeylkeysObjCmd;

/*
 * This mutex protects a static variable which tracks
 * registration of commands and object types.
 */

static Tcl_Mutex initMutex;


/*
 *-----------------------------------------------------------------------------
 *
 * Sv_RegisterKeylistCommands --
 *
 *      Register shared variable commands for TclX keyed lists.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      Memory gets allocated
 *
 *-----------------------------------------------------------------------------
 */
void
Sv_RegisterKeylistCommands(void)
{
    static int initialized;

    if (initialized == 0) {
        Tcl_MutexLock(&initMutex);
        if (initialized == 0) {
            Sv_RegisterCommand("keylset",  SvKeylsetObjCmd,  NULL, 0);
            Sv_RegisterCommand("keylget",  SvKeylgetObjCmd,  NULL, 0);
            Sv_RegisterCommand("keyldel",  SvKeyldelObjCmd,  NULL, 0);
            Sv_RegisterCommand("keylkeys", SvKeylkeysObjCmd, NULL, 0);
            Sv_RegisterObjType(&keyedListType, DupKeyedListInternalRepShared);
            initialized = 1;
        }
        Tcl_MutexUnlock(&initMutex);
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvKeylsetObjCmd --
 *
 *      This procedure is invoked to process the "tsv::keylset" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvKeylsetObjCmd(arg, interp, objc, objv)
    ClientData arg;                     /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int i, off, ret, flg;
    char *key;
    Tcl_Obj *val;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          sv::keylset array lkey key value ?key value ...?
     *          $keylist keylset key value ?key value ...?
     */

    flg = FLAGS_CREATEARRAY | FLAGS_CREATEVAR;
    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, flg);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc - off) < 2 || ((objc - off) % 2)) {
        Tcl_WrongNumArgs(interp, off, objv, "key value ?key value ...?");
        goto cmd_err;
    }
    for (i = off; i < objc; i += 2) {
        key = Tcl_GetString(objv[i]);
        val = Sv_DuplicateObj(objv[i+1]);
        ret = TclX_KeyedListSet(interp, svObj->tclObj, key, val);
        if (ret != TCL_OK) {
            goto cmd_err;
        }
    }

    return Sv_PutContainer(interp, svObj, SV_CHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvKeylgetObjCmd --
 *
 *      This procedure is invoked to process the "tsv::keylget" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvKeylgetObjCmd(arg, interp, objc, objv)
    ClientData arg;                     /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int ret, flg, off;
    char *key;
    Tcl_Obj *varObjPtr = NULL, *valObjPtr = NULL;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          sv::keylget array lkey ?key? ?var?
     *          $keylist keylget ?key? ?var?
     */

    flg = FLAGS_CREATEARRAY | FLAGS_CREATEVAR;
    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, flg);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc - off) > 2) {
        Tcl_WrongNumArgs(interp, off, objv, "?key? ?var?");
        goto cmd_err;
    }
    if ((objc - off) == 0) {
        if (Sv_PutContainer(interp, svObj, SV_UNCHANGED) != TCL_OK) {
            return TCL_ERROR;
        }
        return SvKeylkeysObjCmd(arg, interp, objc, objv);
    }
    if ((objc - off) == 2) {
        varObjPtr = objv[off+1];
    } else {
        varObjPtr = NULL;
    }

    key = Tcl_GetString(objv[off]);
    ret = TclX_KeyedListGet(interp, svObj->tclObj, key, &valObjPtr);
    if (ret == TCL_ERROR) {
        goto cmd_err;
    }

    if (ret == TCL_BREAK) {
        if (varObjPtr) {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(0));
        } else {
            Tcl_AppendResult (interp, "key \"", key, "\" not found", NULL);
            goto cmd_err;
        }
    } else {
        Tcl_Obj *resObjPtr = Sv_DuplicateObj(valObjPtr);
        if (varObjPtr) {
            Tcl_SetObjResult(interp, Tcl_NewIntObj(1));
            Tcl_GetString(varObjPtr);
            if (varObjPtr->length) {
                Tcl_ObjSetVar2(interp, varObjPtr, NULL, resObjPtr, 0);
            }
        } else {
            Tcl_SetObjResult(interp, resObjPtr);
        }
    }

    return Sv_PutContainer(interp, svObj, SV_UNCHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvKeyldelObjCmd --
 *
 *      This procedure is invoked to process the "tsv::keyldel" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvKeyldelObjCmd(arg, interp, objc, objv)
    ClientData arg;                     /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int i, off, ret;
    char *key;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          sv::keyldel array lkey key ?key ...?
     *          $keylist keyldel ?key ...?
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc - off) < 1) {
        Tcl_WrongNumArgs(interp, off, objv, "key ?key ...?");
        goto cmd_err;
    }
    for (i = off; i < objc; i++) {
        key = Tcl_GetString(objv[i]);
        ret = TclX_KeyedListDelete(interp, svObj->tclObj, key);
        if (ret == TCL_BREAK) {
            Tcl_AppendResult(interp, "key \"", key, "\" not found", NULL);
        }
        if (ret == TCL_BREAK || ret == TCL_ERROR) {
            goto cmd_err;
        }
    }

    return Sv_PutContainer(interp, svObj, SV_CHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/*
 *-----------------------------------------------------------------------------
 *
 * SvKeylkeysObjCmd --
 *
 *      This procedure is invoked to process the "tsv::keylkeys" command.
 *      See the user documentation for details on what it does.
 *
 * Results:
 *      A standard Tcl result.
 *
 * Side effects:
 *      See the user documentation.
 *
 *-----------------------------------------------------------------------------
 */

static int
SvKeylkeysObjCmd(arg, interp, objc, objv)
    ClientData arg;                     /* Not used. */
    Tcl_Interp *interp;                 /* Current interpreter. */
    int objc;                           /* Number of arguments. */
    Tcl_Obj *const objv[];              /* Argument objects. */
{
    int ret, off;
    char *key = NULL;
    Tcl_Obj *listObj = NULL;
    Container *svObj = (Container*)arg;

    /*
     * Syntax:
     *          sv::keylkeys array lkey ?key?
     *          $keylist keylkeys ?key?
     */

    ret = Sv_GetContainer(interp, objc, objv, &svObj, &off, 0);
    if (ret != TCL_OK) {
        return TCL_ERROR;
    }
    if ((objc - off) > 1) {
         Tcl_WrongNumArgs(interp, 1, objv, "?lkey?");
         goto cmd_err;
    }
    if ((objc - off) == 1) {
        key = Tcl_GetString(objv[off]);
    }

    ret = TclX_KeyedListGetKeys(interp, svObj->tclObj, key, &listObj);

    if (key && ret == TCL_BREAK) {
        Tcl_AppendResult(interp, "key \"", key, "\" not found", NULL);
    }
    if (ret == TCL_BREAK || ret == TCL_ERROR) {
        goto cmd_err;
    }

    Tcl_SetObjResult (interp, listObj); /* listObj allocated by API !*/

    return Sv_PutContainer(interp, svObj, SV_UNCHANGED);

 cmd_err:
    return Sv_PutContainer(interp, svObj, SV_ERROR);
}

/* EOF $RCSfile: threadSvKeylistCmd.c,v $ */

/* Emacs Setup Variables */
/* Local Variables:      */
/* mode: C               */
/* indent-tabs-mode: nil */
/* c-basic-offset: 4     */
/* End:                  */

