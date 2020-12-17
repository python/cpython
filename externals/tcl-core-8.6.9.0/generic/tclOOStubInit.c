/*
 * This file is (mostly) automatically generated from tclOO.decls.
 * It is compiled and linked in with the tclOO package proper.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "tclOOInt.h"

MODULE_SCOPE const TclOOStubs tclOOStubs;

#ifdef __GNUC__
#pragma GCC dependency "tclOO.decls"
#endif

/* !BEGIN!: Do not edit below this line. */

static const TclOOIntStubs tclOOIntStubs = {
    TCL_STUB_MAGIC,
    0,
    TclOOGetDefineCmdContext, /* 0 */
    TclOOMakeProcInstanceMethod, /* 1 */
    TclOOMakeProcMethod, /* 2 */
    TclOONewProcInstanceMethod, /* 3 */
    TclOONewProcMethod, /* 4 */
    TclOOObjectCmdCore, /* 5 */
    TclOOIsReachable, /* 6 */
    TclOONewForwardMethod, /* 7 */
    TclOONewForwardInstanceMethod, /* 8 */
    TclOONewProcInstanceMethodEx, /* 9 */
    TclOONewProcMethodEx, /* 10 */
    TclOOInvokeObject, /* 11 */
    TclOOObjectSetFilters, /* 12 */
    TclOOClassSetFilters, /* 13 */
    TclOOObjectSetMixins, /* 14 */
    TclOOClassSetMixins, /* 15 */
};

static const TclOOStubHooks tclOOStubHooks = {
    &tclOOIntStubs
};

const TclOOStubs tclOOStubs = {
    TCL_STUB_MAGIC,
    &tclOOStubHooks,
    Tcl_CopyObjectInstance, /* 0 */
    Tcl_GetClassAsObject, /* 1 */
    Tcl_GetObjectAsClass, /* 2 */
    Tcl_GetObjectCommand, /* 3 */
    Tcl_GetObjectFromObj, /* 4 */
    Tcl_GetObjectNamespace, /* 5 */
    Tcl_MethodDeclarerClass, /* 6 */
    Tcl_MethodDeclarerObject, /* 7 */
    Tcl_MethodIsPublic, /* 8 */
    Tcl_MethodIsType, /* 9 */
    Tcl_MethodName, /* 10 */
    Tcl_NewInstanceMethod, /* 11 */
    Tcl_NewMethod, /* 12 */
    Tcl_NewObjectInstance, /* 13 */
    Tcl_ObjectDeleted, /* 14 */
    Tcl_ObjectContextIsFiltering, /* 15 */
    Tcl_ObjectContextMethod, /* 16 */
    Tcl_ObjectContextObject, /* 17 */
    Tcl_ObjectContextSkippedArgs, /* 18 */
    Tcl_ClassGetMetadata, /* 19 */
    Tcl_ClassSetMetadata, /* 20 */
    Tcl_ObjectGetMetadata, /* 21 */
    Tcl_ObjectSetMetadata, /* 22 */
    Tcl_ObjectContextInvokeNext, /* 23 */
    Tcl_ObjectGetMethodNameMapper, /* 24 */
    Tcl_ObjectSetMethodNameMapper, /* 25 */
    Tcl_ClassSetConstructor, /* 26 */
    Tcl_ClassSetDestructor, /* 27 */
    Tcl_GetObjectName, /* 28 */
};

/* !END!: Do not edit above this line. */
