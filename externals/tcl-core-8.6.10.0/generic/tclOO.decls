# tclOO.decls --
#
#	This file contains the declarations for all supported public functions
#	that are exported by the TclOO package that is embedded within the Tcl
#	library via the stubs table.  This file is used to generate the
#	tclOODecls.h, tclOOIntDecls.h and tclOOStubInit.c files.
#
# Copyright (c) 2008-2013 by Donal K. Fellows.
#
# See the file "license.terms" for information on usage and redistribution of
# this file, and for a DISCLAIMER OF ALL WARRANTIES.

library tclOO

######################################################################
# Public API, exposed for general users of TclOO.
#

interface tclOO
hooks tclOOInt
scspec TCLAPI

declare 0 {
    Tcl_Object Tcl_CopyObjectInstance(Tcl_Interp *interp,
	    Tcl_Object sourceObject, const char *targetName,
	    const char *targetNamespaceName)
}
declare 1 {
    Tcl_Object Tcl_GetClassAsObject(Tcl_Class clazz)
}
declare 2 {
    Tcl_Class Tcl_GetObjectAsClass(Tcl_Object object)
}
declare 3 {
    Tcl_Command Tcl_GetObjectCommand(Tcl_Object object)
}
declare 4 {
    Tcl_Object Tcl_GetObjectFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 5 {
    Tcl_Namespace *Tcl_GetObjectNamespace(Tcl_Object object)
}
declare 6 {
    Tcl_Class Tcl_MethodDeclarerClass(Tcl_Method method)
}
declare 7 {
    Tcl_Object Tcl_MethodDeclarerObject(Tcl_Method method)
}
declare 8 {
    int Tcl_MethodIsPublic(Tcl_Method method)
}
declare 9 {
    int Tcl_MethodIsType(Tcl_Method method, const Tcl_MethodType *typePtr,
	    ClientData *clientDataPtr)
}
declare 10 {
    Tcl_Obj *Tcl_MethodName(Tcl_Method method)
}
declare 11 {
    Tcl_Method Tcl_NewInstanceMethod(Tcl_Interp *interp, Tcl_Object object,
	    Tcl_Obj *nameObj, int isPublic, const Tcl_MethodType *typePtr,
	    ClientData clientData)
}
declare 12 {
    Tcl_Method Tcl_NewMethod(Tcl_Interp *interp, Tcl_Class cls,
	    Tcl_Obj *nameObj, int isPublic, const Tcl_MethodType *typePtr,
	    ClientData clientData)
}
declare 13 {
    Tcl_Object Tcl_NewObjectInstance(Tcl_Interp *interp, Tcl_Class cls,
	    const char *nameStr, const char *nsNameStr, int objc,
	    Tcl_Obj *const *objv, int skip)
}
declare 14 {
    int Tcl_ObjectDeleted(Tcl_Object object)
}
declare 15 {
    int Tcl_ObjectContextIsFiltering(Tcl_ObjectContext context)
}
declare 16 {
    Tcl_Method Tcl_ObjectContextMethod(Tcl_ObjectContext context)
}
declare 17 {
    Tcl_Object Tcl_ObjectContextObject(Tcl_ObjectContext context)
}
declare 18 {
    int Tcl_ObjectContextSkippedArgs(Tcl_ObjectContext context)
}
declare 19 {
    ClientData Tcl_ClassGetMetadata(Tcl_Class clazz,
	    const Tcl_ObjectMetadataType *typePtr)
}
declare 20 {
    void Tcl_ClassSetMetadata(Tcl_Class clazz,
	    const Tcl_ObjectMetadataType *typePtr, ClientData metadata)
}
declare 21 {
    ClientData Tcl_ObjectGetMetadata(Tcl_Object object,
	    const Tcl_ObjectMetadataType *typePtr)
}
declare 22 {
    void Tcl_ObjectSetMetadata(Tcl_Object object,
	    const Tcl_ObjectMetadataType *typePtr, ClientData metadata)
}
declare 23 {
    int Tcl_ObjectContextInvokeNext(Tcl_Interp *interp,
	    Tcl_ObjectContext context, int objc, Tcl_Obj *const *objv,
	    int skip)
}
declare 24 {
    Tcl_ObjectMapMethodNameProc *Tcl_ObjectGetMethodNameMapper(
	    Tcl_Object object)
}
declare 25 {
    void Tcl_ObjectSetMethodNameMapper(Tcl_Object object,
	    Tcl_ObjectMapMethodNameProc *mapMethodNameProc)
}
declare 26 {
    void Tcl_ClassSetConstructor(Tcl_Interp *interp, Tcl_Class clazz,
	    Tcl_Method method)
}
declare 27 {
    void Tcl_ClassSetDestructor(Tcl_Interp *interp, Tcl_Class clazz,
	    Tcl_Method method)
}
declare 28 {
    Tcl_Obj *Tcl_GetObjectName(Tcl_Interp *interp, Tcl_Object object)
}

######################################################################
# Private API, exposed to support advanced OO systems that plug in on top of
# TclOO; not intended for general use and does not have any commitment to
# long-term support.
#

interface tclOOInt

declare 0 {
    Tcl_Object TclOOGetDefineCmdContext(Tcl_Interp *interp)
}
declare 1 {
    Tcl_Method TclOOMakeProcInstanceMethod(Tcl_Interp *interp, Object *oPtr,
	    int flags, Tcl_Obj *nameObj, Tcl_Obj *argsObj, Tcl_Obj *bodyObj,
	    const Tcl_MethodType *typePtr, ClientData clientData,
	    Proc **procPtrPtr)
}
declare 2 {
    Tcl_Method TclOOMakeProcMethod(Tcl_Interp *interp, Class *clsPtr,
	    int flags, Tcl_Obj *nameObj, const char *namePtr,
	    Tcl_Obj *argsObj, Tcl_Obj *bodyObj, const Tcl_MethodType *typePtr,
	    ClientData clientData, Proc **procPtrPtr)
}
declare 3 {
    Method *TclOONewProcInstanceMethod(Tcl_Interp *interp, Object *oPtr,
	    int flags, Tcl_Obj *nameObj, Tcl_Obj *argsObj, Tcl_Obj *bodyObj,
	    ProcedureMethod **pmPtrPtr)
}
declare 4 {
    Method *TclOONewProcMethod(Tcl_Interp *interp, Class *clsPtr,
	    int flags, Tcl_Obj *nameObj, Tcl_Obj *argsObj, Tcl_Obj *bodyObj,
	    ProcedureMethod **pmPtrPtr)
}
declare 5 {
    int TclOOObjectCmdCore(Object *oPtr, Tcl_Interp *interp, int objc,
	    Tcl_Obj *const *objv, int publicOnly, Class *startCls)
}
declare 6 {
    int TclOOIsReachable(Class *targetPtr, Class *startPtr)
}
declare 7 {
    Method *TclOONewForwardMethod(Tcl_Interp *interp, Class *clsPtr,
	    int isPublic, Tcl_Obj *nameObj, Tcl_Obj *prefixObj)
}
declare 8 {
    Method *TclOONewForwardInstanceMethod(Tcl_Interp *interp, Object *oPtr,
	    int isPublic, Tcl_Obj *nameObj, Tcl_Obj *prefixObj)
}
declare 9 {
    Tcl_Method TclOONewProcInstanceMethodEx(Tcl_Interp *interp,
	    Tcl_Object oPtr, TclOO_PreCallProc *preCallPtr,
	    TclOO_PostCallProc *postCallPtr, ProcErrorProc *errProc,
	    ClientData clientData, Tcl_Obj *nameObj, Tcl_Obj *argsObj,
	    Tcl_Obj *bodyObj, int flags, void **internalTokenPtr)
}
declare 10 {
    Tcl_Method TclOONewProcMethodEx(Tcl_Interp *interp, Tcl_Class clsPtr,
	    TclOO_PreCallProc *preCallPtr, TclOO_PostCallProc *postCallPtr,
	    ProcErrorProc *errProc, ClientData clientData, Tcl_Obj *nameObj,
	    Tcl_Obj *argsObj, Tcl_Obj *bodyObj, int flags,
	    void **internalTokenPtr)
}
declare 11 {
    int TclOOInvokeObject(Tcl_Interp *interp, Tcl_Object object,
	    Tcl_Class startCls, int publicPrivate, int objc,
	    Tcl_Obj *const *objv)
}
declare 12 {
    void TclOOObjectSetFilters(Object *oPtr, int numFilters,
	    Tcl_Obj *const *filters)
}
declare 13 {
    void TclOOClassSetFilters(Tcl_Interp *interp, Class *classPtr,
	    int numFilters, Tcl_Obj *const *filters)
}
declare 14 {
    void TclOOObjectSetMixins(Object *oPtr, int numMixins,
	    Class *const *mixins)
}
declare 15 {
    void TclOOClassSetMixins(Tcl_Interp *interp, Class *classPtr,
	    int numMixins, Class *const *mixins)
}

return

# Local Variables:
# mode: tcl
# End:
