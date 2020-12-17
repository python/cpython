/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  [incr Tcl] provides object-oriented extensions to Tcl, much as
 *  C++ provides object-oriented extensions to C.  It provides a means
 *  of encapsulating related procedures together with their shared data
 *  in a local namespace that is hidden from the outside world.  It
 *  promotes code re-use through inheritance.  More than anything else,
 *  it encourages better organization of Tcl applications through the
 *  object-oriented paradigm, leading to code that is easier to
 *  understand and maintain.
 *
 *  This part adds a mechanism for integrating C procedures into
 *  [incr Tcl] classes as methods and procs.  Each C procedure must
 *  either be declared via Itcl_RegisterC() or dynamically loaded.
 *
 * ========================================================================
 *  AUTHOR:  Michael J. McLennan
 *           Bell Labs Innovations for Lucent Technologies
 *           mmclennan@lucent.com
 *           http://www.tcltk.com/itcl
 *
 *  overhauled version author: Arnulf Wiedemann
 * ========================================================================
 *           Copyright (c) 1993-1998  Lucent Technologies, Inc.
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#include "itclInt.h"

/*
 *  These records store the pointers for all "RegisterC" functions.
 */
typedef struct ItclCfunc {
    Tcl_CmdProc *argCmdProc;        /* old-style (argc,argv) command handler */
    Tcl_ObjCmdProc *objCmdProc;     /* new (objc,objv) command handler */
    ClientData clientData;          /* client data passed into this function */
    Tcl_CmdDeleteProc *deleteProc;  /* proc called to free clientData */
} ItclCfunc;

static Tcl_HashTable* ItclGetRegisteredProcs(Tcl_Interp *interp);
static void ItclFreeC(ClientData clientData, Tcl_Interp *interp);


/*
 * ------------------------------------------------------------------------
 *  Itcl_RegisterC()
 *
 *  Used to associate a symbolic name with an (argc,argv) C procedure
 *  that handles a Tcl command.  Procedures that are registered in this
 *  manner can be referenced in the body of an [incr Tcl] class
 *  definition to specify C procedures to acting as methods/procs.
 *  Usually invoked in an initialization routine for an extension,
 *  called out in Tcl_AppInit() at the start of an application.
 *
 *  Each symbolic procedure can have an arbitrary client data value
 *  associated with it.  This value is passed into the command
 *  handler whenever it is invoked.
 *
 *  A symbolic procedure name can be used only once for a given style
 *  (arg/obj) handler.  If the name is defined with an arg-style
 *  handler, it can be redefined with an obj-style handler; or if
 *  the name is defined with an obj-style handler, it can be redefined
 *  with an arg-style handler.  In either case, any previous client
 *  data is discarded and the new client data is remembered.  However,
 *  if a name is redefined to a different handler of the same style,
 *  this procedure returns an error.
 *
 *  Returns TCL_OK on success, or TCL_ERROR (along with an error message
 *  in interp->result) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
Itcl_RegisterC(interp, name, proc, clientData, deleteProc)
    Tcl_Interp *interp;             /* interpreter handling this registration */
    const char *name;               /* symbolic name for procedure */
    Tcl_CmdProc *proc;              /* procedure handling Tcl command */
    ClientData clientData;          /* client data associated with proc */
    Tcl_CmdDeleteProc *deleteProc;  /* proc called to free up client data */
{
    int newEntry;
    Tcl_HashEntry *entry;
    Tcl_HashTable *procTable;
    ItclCfunc *cfunc;

    /*
     *  Make sure that a proc was specified.
     */
    if (!proc) {
        Tcl_AppendResult(interp, "initialization error: null pointer for ",
            "C procedure \"", name, "\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    /*
     *  Add a new entry for the given procedure.  If an entry with
     *  this name already exists, then make sure that it was defined
     *  with the same proc.
     */
    procTable = ItclGetRegisteredProcs(interp);
    entry = Tcl_CreateHashEntry(procTable, name, &newEntry);
    if (!newEntry) {
        cfunc = (ItclCfunc*)Tcl_GetHashValue(entry);
        if (cfunc->argCmdProc != NULL && cfunc->argCmdProc != proc) {
            Tcl_AppendResult(interp, "initialization error: C procedure ",
                "with name \"", name, "\" already defined",
                (char*)NULL);
            return TCL_ERROR;
        }

        if (cfunc->deleteProc != NULL) {
            (*cfunc->deleteProc)(cfunc->clientData);
        }
    } else {
        cfunc = (ItclCfunc*)ckalloc(sizeof(ItclCfunc));
        cfunc->objCmdProc = NULL;
    }

    cfunc->argCmdProc = proc;
    cfunc->clientData = clientData;
    cfunc->deleteProc = deleteProc;

    Tcl_SetHashValue(entry, (ClientData)cfunc);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_RegisterObjC()
 *
 *  Used to associate a symbolic name with an (objc,objv) C procedure
 *  that handles a Tcl command.  Procedures that are registered in this
 *  manner can be referenced in the body of an [incr Tcl] class
 *  definition to specify C procedures to acting as methods/procs.
 *  Usually invoked in an initialization routine for an extension,
 *  called out in Tcl_AppInit() at the start of an application.
 *
 *  Each symbolic procedure can have an arbitrary client data value
 *  associated with it.  This value is passed into the command
 *  handler whenever it is invoked.
 *
 *  A symbolic procedure name can be used only once for a given style
 *  (arg/obj) handler.  If the name is defined with an arg-style
 *  handler, it can be redefined with an obj-style handler; or if
 *  the name is defined with an obj-style handler, it can be redefined
 *  with an arg-style handler.  In either case, any previous client
 *  data is discarded and the new client data is remembered.  However,
 *  if a name is redefined to a different handler of the same style,
 *  this procedure returns an error.
 *
 *  Returns TCL_OK on success, or TCL_ERROR (along with an error message
 *  in interp->result) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
Itcl_RegisterObjC(interp, name, proc, clientData, deleteProc)
    Tcl_Interp *interp;     /* interpreter handling this registration */
    const char *name;       /* symbolic name for procedure */
    Tcl_ObjCmdProc *proc;   /* procedure handling Tcl command */
    ClientData clientData;          /* client data associated with proc */
    Tcl_CmdDeleteProc *deleteProc;  /* proc called to free up client data */
{
    int newEntry;
    Tcl_HashEntry *entry;
    Tcl_HashTable *procTable;
    ItclCfunc *cfunc;

    /*
     *  Make sure that a proc was specified.
     */
    if (!proc) {
        Tcl_AppendResult(interp, "initialization error: null pointer for ",
            "C procedure \"", name, "\"",
            (char*)NULL);
        return TCL_ERROR;
    }

    /*
     *  Add a new entry for the given procedure.  If an entry with
     *  this name already exists, then make sure that it was defined
     *  with the same proc.
     */
    procTable = ItclGetRegisteredProcs(interp);
    entry = Tcl_CreateHashEntry(procTable, name, &newEntry);
    if (!newEntry) {
        cfunc = (ItclCfunc*)Tcl_GetHashValue(entry);
        if (cfunc->objCmdProc != NULL && cfunc->objCmdProc != proc) {
            Tcl_AppendResult(interp, "initialization error: C procedure ",
                "with name \"", name, "\" already defined",
                (char*)NULL);
            return TCL_ERROR;
        }

        if (cfunc->deleteProc != NULL) {
            (*cfunc->deleteProc)(cfunc->clientData);
        }
    }
    else {
        cfunc = (ItclCfunc*)ckalloc(sizeof(ItclCfunc));
        cfunc->argCmdProc = NULL;
    }

    cfunc->objCmdProc = proc;
    cfunc->clientData = clientData;
    cfunc->deleteProc = deleteProc;

    Tcl_SetHashValue(entry, (ClientData)cfunc);
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_FindC()
 *
 *  Used to query a C procedure via its symbolic name.  Looks at the
 *  list of procedures registered previously by either Itcl_RegisterC
 *  or Itcl_RegisterObjC and returns pointers to the appropriate
 *  (argc,argv) or (objc,objv) handlers.  Returns non-zero if the
 *  name is recognized and pointers are returned; returns zero
 *  otherwise.
 * ------------------------------------------------------------------------
 */
int
Itcl_FindC(
    Tcl_Interp *interp,           /* interpreter handling this registration */
    const char *name,             /* symbolic name for procedure */
    Tcl_CmdProc **argProcPtr,     /* returns (argc,argv) command handler */
    Tcl_ObjCmdProc **objProcPtr,  /* returns (objc,objv) command handler */
    ClientData *cDataPtr)         /* returns client data */
{
    Tcl_HashEntry *entry;
    Tcl_HashTable *procTable;
    ItclCfunc *cfunc;

    *argProcPtr = NULL;  /* assume info won't be found */
    *objProcPtr = NULL;
    *cDataPtr   = NULL;

    if (interp) {
        procTable = (Tcl_HashTable*)Tcl_GetAssocData(interp,
            "itcl_RegC", (Tcl_InterpDeleteProc**)NULL);

        if (procTable) {
            entry = Tcl_FindHashEntry(procTable, name);
            if (entry) {
                cfunc = (ItclCfunc*)Tcl_GetHashValue(entry);
                *argProcPtr = cfunc->argCmdProc;
                *objProcPtr = cfunc->objCmdProc;
                *cDataPtr   = cfunc->clientData;
            }
        }
    }
    return (*argProcPtr != NULL || *objProcPtr != NULL);
}


/*
 * ------------------------------------------------------------------------
 *  ItclGetRegisteredProcs()
 *
 *  Returns a pointer to a hash table containing the list of registered
 *  procs in the specified interpreter.  If the hash table does not
 *  already exist, it is created.
 * ------------------------------------------------------------------------
 */
static Tcl_HashTable*
ItclGetRegisteredProcs(interp)
    Tcl_Interp *interp;  /* interpreter handling this registration */
{
    Tcl_HashTable* procTable;

    /*
     *  If the registration table does not yet exist, then create it.
     */
    procTable = (Tcl_HashTable*)Tcl_GetAssocData(interp, "itcl_RegC",
        (Tcl_InterpDeleteProc**)NULL);

    if (!procTable) {
        procTable = (Tcl_HashTable*)ckalloc(sizeof(Tcl_HashTable));
        Tcl_InitHashTable(procTable, TCL_STRING_KEYS);
        Tcl_SetAssocData(interp, "itcl_RegC", ItclFreeC,
            (ClientData)procTable);
    }
    return procTable;
}


/*
 * ------------------------------------------------------------------------
 *  ItclFreeC()
 *
 *  When an interpreter is deleted, this procedure is called to
 *  free up the associated data created by Itcl_RegisterC and
 *  Itcl_RegisterObjC.
 * ------------------------------------------------------------------------
 */
static void
ItclFreeC(clientData, interp)
    ClientData clientData;       /* associated data */
    Tcl_Interp *interp;          /* intepreter being deleted */
{
    Tcl_HashTable *tablePtr = (Tcl_HashTable*)clientData;
    Tcl_HashSearch place;
    Tcl_HashEntry *entry;
    ItclCfunc *cfunc;

    entry = Tcl_FirstHashEntry(tablePtr, &place);
    while (entry) {
        cfunc = (ItclCfunc*)Tcl_GetHashValue(entry);

        if (cfunc->deleteProc != NULL) {
            (*cfunc->deleteProc)(cfunc->clientData);
        }
        ckfree ( (char*)cfunc );
        entry = Tcl_NextHashEntry(&place);
    }

    Tcl_DeleteHashTable(tablePtr);
    ckfree((char*)tablePtr);
}
