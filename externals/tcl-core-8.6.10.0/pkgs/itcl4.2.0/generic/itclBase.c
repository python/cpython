/*
 * itclBase.c --
 *
 * This file contains the C-implemented startup part of an
 * Itcl implemenatation
 *
 * Copyright (c) 2007 by Arnulf P. Wiedemann
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <stdlib.h>
#include "itclInt.h"

static Tcl_NamespaceDeleteProc FreeItclObjectInfo;
static Tcl_ObjCmdProc ItclSetHullWindowName;
static Tcl_ObjCmdProc ItclCheckSetItclHull;

MODULE_SCOPE const ItclStubs itclStubs;

static int Initialize(Tcl_Interp *interp);

static const char initScript[] =
"namespace eval ::itcl {\n"
"    proc _find_init {} {\n"
"        global env tcl_library\n"
"        variable library\n"
"        variable patchLevel\n"
"        rename _find_init {}\n"
"        if {[info exists library]} {\n"
"            lappend dirs $library\n"
"        } else {\n"
"            set dirs {}\n"
"            if {[info exists env(ITCL_LIBRARY)]} {\n"
"                lappend dirs $env(ITCL_LIBRARY)\n"
"            }\n"
"            lappend dirs [file join [file dirname $tcl_library] itcl$patchLevel]\n"
"            set bindir [file dirname [info nameofexecutable]]\n"
"            lappend dirs [file join . library]\n"
"            lappend dirs [file join $bindir .. lib itcl$patchLevel]\n"
"            lappend dirs [file join $bindir .. library]\n"
"            lappend dirs [file join $bindir .. .. library]\n"
"            lappend dirs [file join $bindir .. .. itcl library]\n"
"            lappend dirs [file join $bindir .. .. .. itcl library]\n"
"            lappend dirs [file join $bindir .. .. itcl-ng itcl library]\n"
"            # On *nix, check the directories in the tcl_pkgPath\n"
"            # XXX JH - this looks unnecessary, maybe Darwin only?\n"
"            if {[string equal $::tcl_platform(platform) \"unix\"]} {\n"
"                foreach d $::tcl_pkgPath {\n"
"                    lappend dirs $d\n"
"                    lappend dirs [file join $d itcl$patchLevel]\n"
"                }\n"
"            }\n"
"        }\n"
"        foreach i $dirs {\n"
"            set library $i\n"
"            if {![catch {uplevel #0 [list source [file join $i itcl.tcl]]}]} {\n"
"                set library $i\n"
"                return\n"
"            }\n"
"        }\n"
"        set msg \"Can't find a usable itcl.tcl in the following directories:\n\"\n"
"        append msg \"    $dirs\n\"\n"
"        append msg \"This probably means that Itcl/Tcl weren't installed properly.\n\"\n"
"        append msg \"If you know where the Itcl library directory was installed,\n\"\n"
"        append msg \"you can set the environment variable ITCL_LIBRARY to point\n\"\n"
"        append msg \"to the library directory.\n\"\n"
"        error $msg\n"
"    }\n"
"    _find_init\n"
"}";

/*
 * The following script is used to initialize Itcl in a safe interpreter.
 */

static const char safeInitScript[] =
"proc ::itcl::local {class name args} {\n"
"    set ptr [uplevel [list $class $name] $args]\n"
"    uplevel [list set itcl-local-$ptr $ptr]\n"
"    set cmd [uplevel namespace which -command $ptr]\n"
"    uplevel [list trace variable itcl-local-$ptr u \"::itcl::delete object $cmd; list\"]\n"
"    return $ptr\n"
"}";

static const char *clazzClassScript =
"::oo::class create ::itcl::clazz {\n"
"  superclass ::oo::class\n"
"  method unknown args {\n"
"    ::tailcall ::itcl::parser::handleClass [::lindex [::info level 0] 0] [self] {*}$args\n"
"  }\n"
"  unexport create new unknown\n"
"}";

#define ITCL_IS_ENSEMBLE 0x1

#ifdef ITCL_DEBUG_C_INTERFACE
extern void RegisterDebugCFunctions( Tcl_Interp * interp);
#endif

static Tcl_ObjectMetadataDeleteProc Demolition;

static const Tcl_ObjectMetadataType canary = {
    TCL_OO_METADATA_VERSION_CURRENT,
    "Itcl Foundations",
    Demolition,
    NULL
};

void
Demolition(
    void *clientData)
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)clientData;

    infoPtr->clazzObjectPtr = NULL;
    infoPtr->clazzClassPtr = NULL;
}

static const Tcl_ObjectMetadataType objMDT = {
    TCL_OO_METADATA_VERSION_CURRENT,
    "ItclObject",
    ItclDeleteObjectMetadata,	/* Not really used yet */
    NULL
};

static Tcl_MethodCallProc RootCallProc;

const Tcl_MethodType itclRootMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
    "itcl root method",
    RootCallProc,
    NULL,
    NULL
};

static int
RootCallProc(
    void *clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Object oPtr = Tcl_ObjectContextObject(context);
    ItclObject *ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr, &objMDT);
    ItclRootMethodProc *proc = (ItclRootMethodProc *)clientData;

    return (*proc)(ioPtr, interp, objc, objv);
}

/*
 * ------------------------------------------------------------------------
 *  Initialize()
 *
 *      that is the starting point when loading the library
 *      it initializes all internal stuff
 *
 * ------------------------------------------------------------------------
 */

static int
Initialize (
    Tcl_Interp *interp)
{
    Tcl_Namespace *nsPtr;
    Tcl_Namespace *itclNs;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    const char * ret;
    char *res_option;
    int opt;
    int isNew;
    Tcl_Class tclCls;
    Tcl_Object clazzObjectPtr, root;
    Tcl_Obj *objPtr, *resPtr;

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return TCL_ERROR;
    }

    ret = TclOOInitializeStubs(interp, "1.0");
    if (ret == NULL) {
        return TCL_ERROR;
    }

    objPtr = Tcl_NewStringObj("::oo::class", -1);
    Tcl_IncrRefCount(objPtr);
    clazzObjectPtr = Tcl_GetObjectFromObj(interp, objPtr);
    if (!clazzObjectPtr || !(tclCls = Tcl_GetObjectAsClass(clazzObjectPtr))) {
	Tcl_DecrRefCount(objPtr);
        return TCL_ERROR;
    }
    Tcl_DecrRefCount(objPtr);

    infoPtr = (ItclObjectInfo*)Itcl_Alloc(sizeof(ItclObjectInfo));

    nsPtr = Tcl_CreateNamespace(interp, ITCL_NAMESPACE, infoPtr, FreeItclObjectInfo);
    if (nsPtr == NULL) {
	Itcl_Free(infoPtr);
        Tcl_Panic("Itcl: cannot create namespace: \"%s\" \n", ITCL_NAMESPACE);
    }

    nsPtr = Tcl_CreateNamespace(interp, ITCL_INTDICTS_NAMESPACE,
            NULL, NULL);
    if (nsPtr == NULL) {
	Itcl_Free(infoPtr);
        Tcl_Panic("Itcl: cannot create namespace: \"%s::internal::dicts\" \n",
	        ITCL_NAMESPACE);
    }

    /*
     *  Create the top-level data structure for tracking objects.
     *  Store this as "associated data" for easy access, but link
     *  it to the itcl namespace for ownership.
     */
    infoPtr->interp = interp;
    infoPtr->class_meta_type = (Tcl_ObjectMetadataType *)ckalloc(
            sizeof(Tcl_ObjectMetadataType));
    infoPtr->class_meta_type->version = TCL_OO_METADATA_VERSION_CURRENT;
    infoPtr->class_meta_type->name = "ItclClass";
    infoPtr->class_meta_type->deleteProc = ItclDeleteClassMetadata;
    infoPtr->class_meta_type->cloneProc = NULL;

    infoPtr->object_meta_type = &objMDT;

    Tcl_InitHashTable(&infoPtr->objects, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&infoPtr->objectCmds, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&infoPtr->classes, TCL_ONE_WORD_KEYS);
    Tcl_InitObjHashTable(&infoPtr->nameClasses);
    Tcl_InitHashTable(&infoPtr->namespaceClasses, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&infoPtr->procMethods, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&infoPtr->instances, TCL_STRING_KEYS);
    Tcl_InitHashTable(&infoPtr->frameContext, TCL_ONE_WORD_KEYS);
    Tcl_InitObjHashTable(&infoPtr->classTypes);

    infoPtr->ensembleInfo = (EnsembleInfo *)ckalloc(sizeof(EnsembleInfo));
    memset(infoPtr->ensembleInfo, 0, sizeof(EnsembleInfo));
    Tcl_InitHashTable(&infoPtr->ensembleInfo->ensembles, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&infoPtr->ensembleInfo->subEnsembles, TCL_ONE_WORD_KEYS);
    infoPtr->ensembleInfo->numEnsembles = 0;
    infoPtr->protection = ITCL_DEFAULT_PROTECT;
    infoPtr->currClassFlags = 0;
    infoPtr->buildingWidget = 0;
    infoPtr->typeDestructorArgumentPtr = Tcl_NewStringObj("", -1);
    Tcl_IncrRefCount(infoPtr->typeDestructorArgumentPtr);
    infoPtr->lastIoPtr = NULL;

    Tcl_SetVar2(interp, ITCL_NAMESPACE"::internal::dicts::classes", NULL, "", 0);
    Tcl_SetVar2(interp, ITCL_NAMESPACE"::internal::dicts::objects", NULL, "", 0);
    Tcl_SetVar2(interp, ITCL_NAMESPACE"::internal::dicts::classOptions", NULL, "", 0);
    Tcl_SetVar2(interp,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedOptions", NULL, "", 0);
    Tcl_SetVar2(interp,
            ITCL_NAMESPACE"::internal::dicts::classComponents", NULL, "", 0);
    Tcl_SetVar2(interp,
            ITCL_NAMESPACE"::internal::dicts::classVariables", NULL, "", 0);
    Tcl_SetVar2(interp,
            ITCL_NAMESPACE"::internal::dicts::classFunctions", NULL, "", 0);
    Tcl_SetVar2(interp,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedFunctions", NULL, "", 0);

    hPtr = Tcl_CreateHashEntry(&infoPtr->classTypes,
            (char *)Tcl_NewStringObj("class", -1), &isNew);
    Tcl_SetHashValue(hPtr, ITCL_CLASS);
    hPtr = Tcl_CreateHashEntry(&infoPtr->classTypes,
            (char *)Tcl_NewStringObj("type", -1), &isNew);
    Tcl_SetHashValue(hPtr, ITCL_TYPE);
    hPtr = Tcl_CreateHashEntry(&infoPtr->classTypes,
            (char *)Tcl_NewStringObj("widget", -1), &isNew);
    Tcl_SetHashValue(hPtr, ITCL_WIDGET);
    hPtr = Tcl_CreateHashEntry(&infoPtr->classTypes,
            (char *)Tcl_NewStringObj("widgetadaptor", -1), &isNew);
    Tcl_SetHashValue(hPtr, ITCL_WIDGETADAPTOR);
    hPtr = Tcl_CreateHashEntry(&infoPtr->classTypes,
            (char *)Tcl_NewStringObj("extendedclass", -1), &isNew);
    Tcl_SetHashValue(hPtr, ITCL_ECLASS);

    res_option = getenv("ITCL_USE_OLD_RESOLVERS");
    if (res_option == NULL) {
	opt = 1;
    } else {
	opt = atoi(res_option);
    }
    infoPtr->useOldResolvers = opt;
    Itcl_InitStack(&infoPtr->clsStack);

    Tcl_SetAssocData(interp, ITCL_INTERP_DATA, NULL, infoPtr);

    Itcl_PreserveData(infoPtr);

    root = Tcl_NewObjectInstance(interp, tclCls, "::itcl::Root",
	    NULL, 0, NULL, 0);

    Tcl_NewMethod(interp, Tcl_GetObjectAsClass(root),
	    Tcl_NewStringObj("unknown", -1), 0, &itclRootMethodType,
	    (void *)ItclUnknownGuts);
    Tcl_NewMethod(interp, Tcl_GetObjectAsClass(root),
	    Tcl_NewStringObj("ItclConstructBase", -1), 0,
	    &itclRootMethodType, (void *)ItclConstructGuts);
    Tcl_NewMethod(interp, Tcl_GetObjectAsClass(root),
	    Tcl_NewStringObj("info", -1), 1,
	    &itclRootMethodType, (void *)ItclInfoGuts);

    /* first create the Itcl base class as root of itcl classes */
    if (Tcl_EvalEx(interp, clazzClassScript, -1, 0) != TCL_OK) {
        Tcl_Panic("cannot create Itcl root class ::itcl::clazz");
    }
    resPtr = Tcl_GetObjResult(interp);
    /*
     * Tcl_GetObjectFromObject can call Tcl_SetObjResult, so increment the
     * refcount first.
     */
    Tcl_IncrRefCount(resPtr);
    clazzObjectPtr = Tcl_GetObjectFromObj(interp, resPtr);
    Tcl_DecrRefCount(resPtr);

    if (clazzObjectPtr == NULL) {
        Tcl_AppendResult(interp,
                "ITCL: cannot get Object for ::itcl::clazz for class \"",
                "::itcl::clazz", "\"", NULL);
        return TCL_ERROR;
    }

    Tcl_ObjectSetMetadata(clazzObjectPtr, &canary, infoPtr);

    infoPtr->clazzObjectPtr = clazzObjectPtr;
    infoPtr->clazzClassPtr = Tcl_GetObjectAsClass(clazzObjectPtr);

    /*
     *  Initialize the ensemble package first, since we need this
     *  for other parts of [incr Tcl].
     */

    if (Itcl_EnsembleInit(interp) != TCL_OK) {
        return TCL_ERROR;
    }

    Itcl_ParseInit(interp, infoPtr);

    /*
     *  Create "itcl::builtin" namespace for commands that
     *  are automatically built into class definitions.
     */
    if (Itcl_BiInit(interp, infoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    /*
     *  Export all commands in the "itcl" namespace so that they
     *  can be imported with something like "namespace import itcl::*"
     */
    itclNs = Tcl_FindNamespace(interp, "::itcl", NULL,
        TCL_LEAVE_ERR_MSG);

    /*
     *  This was changed from a glob export (itcl::*) to explicit
     *  command exports, so that the itcl::is command can *not* be
     *  exported. This is done for concern that the itcl::is command
     *  imported might be confusing ("is").
     */
    if (!itclNs ||
            (Tcl_Export(interp, itclNs, "body", /* reset */ 1) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "class", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "code", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "configbody", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "delete", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "delete_helper", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "ensemble", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "filter", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "find", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "forward", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "local", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "mixin", 0) != TCL_OK) ||
            (Tcl_Export(interp, itclNs, "scope", 0) != TCL_OK)) {
        return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp,
            ITCL_NAMESPACE"::internal::commands::sethullwindowname",
            ItclSetHullWindowName, infoPtr, NULL);
    Tcl_CreateObjCommand(interp,
            ITCL_NAMESPACE"::internal::commands::checksetitclhull",
            ItclCheckSetItclHull, infoPtr, NULL);

    /*
     *  Set up the variables containing version info.
     */

    Tcl_SetVar2(interp, "::itcl::version", NULL, ITCL_VERSION, TCL_NAMESPACE_ONLY);
    Tcl_SetVar2(interp, "::itcl::patchLevel", NULL, ITCL_PATCH_LEVEL,
            TCL_NAMESPACE_ONLY);


#ifdef ITCL_DEBUG_C_INTERFACE
    RegisterDebugCFunctions(interp);
#endif
    /*
     *  Package is now loaded.
     */

    Tcl_PkgProvideEx(interp, "Itcl", ITCL_PATCH_LEVEL, &itclStubs);
    return Tcl_PkgProvideEx(interp, "itcl", ITCL_PATCH_LEVEL, &itclStubs);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_Init()
 *
 *  Invoked whenever a new INTERPRETER is created to install the
 *  [incr Tcl] package.  Usually invoked within Tcl_AppInit() at
 *  the start of execution.
 *
 *  Creates the "::itcl" namespace and installs access commands for
 *  creating classes and querying info.
 *
 *  Returns TCL_OK on success, or TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */

int
Itcl_Init (
    Tcl_Interp *interp)
{
    if (Initialize(interp) != TCL_OK) {
        return TCL_ERROR;
    }

    return  Tcl_EvalEx(interp, initScript, -1, 0);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_SafeInit()
 *
 *  Invoked whenever a new SAFE INTERPRETER is created to install
 *  the [incr Tcl] package.
 *
 *  Creates the "::itcl" namespace and installs access commands for
 *  creating classes and querying info.
 *
 *  Returns TCL_OK on success, or TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */

int
Itcl_SafeInit (
    Tcl_Interp *interp)
{
    if (Initialize(interp) != TCL_OK) {
        return TCL_ERROR;
    }
    return Tcl_EvalEx(interp, safeInitScript, -1, 0);
}

/*
 * ------------------------------------------------------------------------
 *  ItclSetHullWindowName()
 *
 *
 * ------------------------------------------------------------------------
 */
static int
ItclSetHullWindowName(
    void *clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr;

    infoPtr = (ItclObjectInfo *)clientData;
    if (infoPtr->currIoPtr != NULL) {
        infoPtr->currIoPtr->hullWindowNamePtr = objv[1];
	Tcl_IncrRefCount(infoPtr->currIoPtr->hullWindowNamePtr);
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclCheckSetItclHull()
 *
 *
 * ------------------------------------------------------------------------
 */
static int
ItclCheckSetItclHull(
    void *clientData,   /* infoPtr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *objPtr;
    ItclObject *ioPtr;
    ItclVariable *ivPtr;
    ItclObjectInfo *infoPtr;
    const char *valueStr;

    if (objc < 3) {
        Tcl_AppendResult(interp, "ItclCheckSetItclHull wrong # args should be ",
	        "<objectName> <value>", NULL);
	return TCL_ERROR;
    }

    /*
     * This is an internal command, and is never called with an
     * objectName value other than the empty list. Check that with
     * an assertion so alternative handling can be removed.
     */
    assert( strlen(Tcl_GetString(objv[1])) == 0);
    infoPtr = (ItclObjectInfo *)clientData;
    {
        ioPtr = infoPtr->currIoPtr;
	if (ioPtr == NULL) {
            Tcl_AppendResult(interp, "ItclCheckSetItclHull cannot find object",
	            NULL);
	    return TCL_ERROR;
        }
    }
    objPtr = Tcl_NewStringObj("itcl_hull", -1);
    hPtr = Tcl_FindHashEntry(&ioPtr->iclsPtr->variables, (char *)objPtr);
    Tcl_DecrRefCount(objPtr);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "ItclCheckSetItclHull cannot find itcl_hull",
	        " variable for object \"", Tcl_GetString(objv[1]), "\"", NULL);
	return TCL_ERROR;
    }
    ivPtr = (ItclVariable *)Tcl_GetHashValue(hPtr);
    valueStr = Tcl_GetString(objv[2]);
    if (strcmp(valueStr, "2") == 0) {
        ivPtr->initted = 2;
    } else {
        if (strcmp(valueStr, "0") == 0) {
            ivPtr->initted = 0;
	} else {
            Tcl_AppendResult(interp, "ItclCheckSetItclHull bad value \"",
	            valueStr, "\"", NULL);
	    return TCL_ERROR;
	}
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  FreeItclObjectInfo()
 *
 *  called when an interp is deleted to free up memory
 *
 * ------------------------------------------------------------------------
 */
static void
FreeItclObjectInfo(
    void *clientData)
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)clientData;

    Tcl_DeleteHashTable(&infoPtr->instances);
    Tcl_DeleteHashTable(&infoPtr->classTypes);
    Tcl_DeleteHashTable(&infoPtr->procMethods);
    Tcl_DeleteHashTable(&infoPtr->objectCmds);
    Tcl_DeleteHashTable(&infoPtr->classes);
    Tcl_DeleteHashTable(&infoPtr->nameClasses);
    Tcl_DeleteHashTable(&infoPtr->namespaceClasses);

    assert (infoPtr->infoVarsPtr == NULL);
    assert (infoPtr->infoVars4Ptr == NULL);

    if (infoPtr->typeDestructorArgumentPtr) {
	Tcl_DecrRefCount(infoPtr->typeDestructorArgumentPtr);
	infoPtr->typeDestructorArgumentPtr = NULL;
    }

    /* cleanup ensemble info */
    if (infoPtr->ensembleInfo) {
	Tcl_DeleteHashTable(&infoPtr->ensembleInfo->ensembles);
	Tcl_DeleteHashTable(&infoPtr->ensembleInfo->subEnsembles);
	ItclFinishEnsemble(infoPtr);
	ckfree((char *)infoPtr->ensembleInfo);
	infoPtr->ensembleInfo = NULL;
    }

    if (infoPtr->class_meta_type) {
	ckfree((char *)infoPtr->class_meta_type);
	infoPtr->class_meta_type = NULL;
    }

    /* clean up list pool */
    Itcl_FinishList();

    Itcl_ReleaseData(infoPtr);
}
