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
 *  AUTHOR:  Arnulf Wiedemann
 * ========================================================================
 *           Copyright (c) Arnulf Wiedemann
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
#ifdef ITCL_DEBUG_C_INTERFACE

#include <stdio.h>
#include "itclInt.h"

Tcl_CmdProc cArgFunc;
Tcl_ObjCmdProc cObjFunc;

int
cArgFunc(
    ClientData clientData,
    Tcl_Interp *interp,
    int argc,
    const char **argv)
{
    int result;
    ItclObjectInfo * infoPtr = NULL;
    ItclClass *iclsPtr = NULL;
    ItclClass * classPtr;
    ItclObject * rioPtr = (ItclObject *)1;
    Tcl_Obj * objv[4];
    FOREACH_HASH_DECLS;

//fprintf(stderr, "argc: %d\n", argc);
    if (argc != 4) {
      Tcl_AppendResult(interp, "wrong #args: should be ::itcl::parser::handleClass className className objectName", NULL);
      return TCL_ERROR;
    }
    objv[0] = Tcl_NewStringObj(argv[0], -1);
    objv[1] = Tcl_NewStringObj(argv[1], -1); /* class name */
    objv[2] = Tcl_NewStringObj(argv[2], -1); /* full class name */
    objv[3] = Tcl_NewStringObj(argv[3], -1); /* object name */
    Tcl_IncrRefCount(objv[0]);
    Tcl_IncrRefCount(objv[1]);
    Tcl_IncrRefCount(objv[2]);
    Tcl_IncrRefCount(objv[3]);
    infoPtr = (ClientData)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    FOREACH_HASH_VALUE(classPtr,&infoPtr->nameClasses) {
        if (strcmp(Tcl_GetString(objv[1]), Tcl_GetString(classPtr->fullNamePtr)) == 0 ||
                strcmp(Tcl_GetString(objv[2]), Tcl_GetString(classPtr->fullNamePtr)) == 0) {
           iclsPtr = classPtr;
	   break;
        }
    }
    if (iclsPtr == NULL) {
      Tcl_AppendResult(interp, "no such class: ", Tcl_GetString(objv[2]), NULL);
      return TCL_ERROR;
    }

    /* try to create an object for a class as a test for calling a C function from
     * an Itcl class. See file CreateItclObjectWithC_example.tcl in library directory
     */
    result = Itcl_CreateObject(interp, Tcl_GetString(objv[3]), iclsPtr, 4, objv, &rioPtr);
    return result;
}

int
cObjFunc(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Namespace *nsPtr;
    ItclObjectInfo * infoPtr = NULL;
    ItclClass *iclsPtr = NULL;
    ItclClass * classPtr;
    FOREACH_HASH_DECLS;
    int i;

    ItclShowArgs(0, "cObjFunc called", objc, objv);
fprintf(stderr, "objv: %d %p\n", objc, objv);
for(i = 0; i<objc;i++) {
    fprintf(stderr, "arg:%d:%s:\n", i, Tcl_GetString(objv[i]));
}
    nsPtr = Tcl_GetCurrentNamespace(interp);
fprintf(stderr, "IP:%p %p %p !%s!\n",interp, clientData, nsPtr, nsPtr->fullName);
    infoPtr = (ClientData)Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    FOREACH_HASH_VALUE(classPtr,&infoPtr->nameClasses) {
        if (strcmp(Tcl_GetString(objv[1]), Tcl_GetString(classPtr->fullNamePtr)) == 0 ||
                strcmp(Tcl_GetString(objv[2]), Tcl_GetString(classPtr->fullNamePtr)) == 0) {
           iclsPtr = classPtr;
	   break;
        }
    }
fprintf(stderr, "IP2:%p %p %p\n",interp, clientData, iclsPtr);
    return TCL_OK;
}


void
RegisterDebugCFunctions(Tcl_Interp *interp)
{
    int result;

    /* args: interp, name, c-function, clientdata, deleteproc */
    result = Itcl_RegisterC(interp, "cArgFunc", cArgFunc, NULL, NULL);
    result = Itcl_RegisterObjC(interp, "cObjFunc", cObjFunc, NULL, NULL);
    if (result != 0) {
    }
}
#endif
