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

static Tcl_ObjCmdProc ItclFinishCmd;
static Tcl_ObjCmdProc ItclSetHullWindowName;
static Tcl_ObjCmdProc ItclCheckSetItclHull;

#ifdef OBJ_REF_COUNT_DEBUG
static Tcl_ObjCmdProc ItclDumpRefCountInfo;
#endif

#ifdef ITCL_PRESERVE_DEBUG
static Tcl_ObjCmdProc ItclDumpPreserveInfo;
#endif

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

typedef struct ItclCmdsInfo {
    const char *name;
    int flags;
} ItclCmdsInfo;
static ItclCmdsInfo itclCmds [] = {
    { "::itcl::class", 0},
    { "::itcl::find", ITCL_IS_ENSEMBLE},
    { "::itcl::delete", ITCL_IS_ENSEMBLE},
    { "::itcl::is", ITCL_IS_ENSEMBLE},
    { "::itcl::filter", ITCL_IS_ENSEMBLE},
    { "::itcl::forward", ITCL_IS_ENSEMBLE},
    { "::itcl::import::stub", ITCL_IS_ENSEMBLE},
    { "::itcl::mixin", ITCL_IS_ENSEMBLE},
    { "::itcl::parser::delegate", ITCL_IS_ENSEMBLE},
    { "::itcl::type", 0},
    { "::itcl::widget", 0},
    { "::itcl::widgetadaptor", 0},
    { "::itcl::nwidget", 0},
    { "::itcl::addoption", 0},
    { "::itcl::addobjectoption", 0},
    { "::itcl::adddelegatedoption", 0},
    { "::itcl::adddelegatedmethod", 0},
    { "::itcl::addcomponent", 0},
    { "::itcl::setcomponent", 0},
    { "::itcl::extendedclass", 0},
    { "::itcl::genericclass", 0},
    { "::itcl::parser::delegate", ITCL_IS_ENSEMBLE},
    { NULL, 0},
};
#ifdef ITCL_DEBUG_C_INTERFACE
extern void RegisterDebugCFunctions( Tcl_Interp * interp);
#endif

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
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    Tcl_Object oPtr = Tcl_ObjectContextObject(context);
    ItclObject *ioPtr = Tcl_ObjectGetMetadata(oPtr, &objMDT);
    ItclRootMethodProc *proc = (ItclRootMethodProc *)clientData;

    return (*proc)(ioPtr, interp, objc, objv);
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
    ClientData clientData)
{
    ItclObjectInfo *infoPtr;

    infoPtr = (ItclObjectInfo *)clientData;
    ItclFinishCmd(infoPtr, infoPtr->interp, 0, NULL);
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

#ifdef NEW_PROTO_RESOLVER
int ItclVarsAndCommandResolveInit(Tcl_Interp *interp);
#endif

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
    Tcl_Object clazzObjectPtr, root;
    Tcl_Obj *objPtr, *resPtr;

    if (Tcl_InitStubs(interp, "8.6", 0) == NULL) {
        return TCL_ERROR;
    }

    ret = TclOOInitializeStubs(interp, "1.0");
    if (ret == NULL) {
        return TCL_ERROR;
    }

    infoPtr = (ItclObjectInfo*)ckalloc(sizeof(ItclObjectInfo));

    nsPtr = Tcl_CreateNamespace(interp, ITCL_NAMESPACE, infoPtr, FreeItclObjectInfo);
    if (nsPtr == NULL) {
	ckfree(infoPtr);
        Tcl_Panic("Itcl: cannot create namespace: \"%s\" \n", ITCL_NAMESPACE);
    }

    nsPtr = Tcl_CreateNamespace(interp, ITCL_NAMESPACE"::internal::dicts",
            NULL, NULL);
    if (nsPtr == NULL) {
	ckfree(infoPtr);
        Tcl_Panic("Itcl: cannot create namespace: \"%s::internal::dicts\" \n",
	        ITCL_NAMESPACE);
    }

    Tcl_CreateObjCommand(interp, ITCL_NAMESPACE"::finish", ItclFinishCmd,
            NULL, NULL);

    /* for debugging only !!! */
#ifdef OBJ_REF_COUNT_DEBUG
    Tcl_CreateObjCommand(interp,
            ITCL_NAMESPACE"::dumprefcountinfo",
            ItclDumpRefCountInfo, NULL, NULL);
#endif

#ifdef ITCL_PRESERVE_DEBUG
    Tcl_CreateObjCommand(interp,
            ITCL_NAMESPACE"::dumppreserveinfo",
            ItclDumpPreserveInfo, NULL, NULL);
#endif
    /* END for debugging only !!! */

    /*
     *  Create the top-level data structure for tracking objects.
     *  Store this as "associated data" for easy access, but link
     *  it to the itcl namespace for ownership.
     */
    memset(infoPtr, 0, sizeof(ItclObjectInfo));
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

    Tcl_SetVar(interp, ITCL_NAMESPACE"::internal::dicts::classes", "", 0);
    Tcl_SetVar(interp, ITCL_NAMESPACE"::internal::dicts::objects", "", 0);
    Tcl_SetVar(interp, ITCL_NAMESPACE"::internal::dicts::classOptions", "", 0);
    Tcl_SetVar(interp,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedOptions", "", 0);
    Tcl_SetVar(interp,
            ITCL_NAMESPACE"::internal::dicts::classComponents", "", 0);
    Tcl_SetVar(interp,
            ITCL_NAMESPACE"::internal::dicts::classVariables", "", 0);
    Tcl_SetVar(interp,
            ITCL_NAMESPACE"::internal::dicts::classFunctions", "", 0);
    Tcl_SetVar(interp,
            ITCL_NAMESPACE"::internal::dicts::classDelegatedFunctions", "", 0);

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

    Tcl_SetAssocData(interp, ITCL_INTERP_DATA, NULL, (ClientData)infoPtr);

    Itcl_PreserveData((ClientData)infoPtr);

#ifdef NEW_PROTO_RESOLVER
    ItclVarsAndCommandResolveInit(interp);
#endif

    objPtr = Tcl_NewStringObj("::oo::class", -1);
    root = Tcl_NewObjectInstance(interp, Tcl_GetObjectAsClass(
	    Tcl_GetObjectFromObj(interp, objPtr)), "::itcl::Root",
	    NULL, 0, NULL, 0);
    Tcl_DecrRefCount(objPtr);

    Tcl_NewMethod(interp, Tcl_GetObjectAsClass(root),
	    Tcl_NewStringObj("unknown", -1), 0, &itclRootMethodType,
	    ItclUnknownGuts);
    Tcl_NewMethod(interp, Tcl_GetObjectAsClass(root),
	    Tcl_NewStringObj("ItclConstructBase", -1), 0,
	    &itclRootMethodType, ItclConstructGuts);
    Tcl_NewMethod(interp, Tcl_GetObjectAsClass(root),
	    Tcl_NewStringObj("info", -1), 1,
	    &itclRootMethodType, ItclInfoGuts);

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

    /* work around for SF bug #254 needed because of problem in TclOO 1.0.2 !! */
    if (Tcl_PkgPresent(interp, "TclOO", "1.0.2", 1) != NULL) {
	Itcl_IncrObjectRefCount(clazzObjectPtr);
    }

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
    itclNs = Tcl_FindNamespace(interp, "::itcl", (Tcl_Namespace*)NULL,
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

    Tcl_SetVar(interp, "::itcl::version", ITCL_VERSION, TCL_NAMESPACE_ONLY);
    Tcl_SetVar(interp, "::itcl::patchLevel", ITCL_PATCH_LEVEL,
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
    ClientData clientData,   /* infoPtr */
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
    ClientData clientData,   /* infoPtr */
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
    ivPtr = Tcl_GetHashValue(hPtr);
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
 *  ItclFinishCmd()
 *
 *  called when an interp is deleted to free up memory or called explicitly
 *  to check memory leaks
 *
 * ------------------------------------------------------------------------
 */
static int
ItclFinishCmd(
    ClientData clientData,   /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch place;
    Tcl_Namespace *nsPtr;
    Tcl_Obj **newObjv;
    Tcl_Obj *objPtr;
    Tcl_Obj *ensObjPtr;
    Tcl_Command cmdPtr;
    Tcl_Obj *mapDict;
    ItclObjectInfo *infoPtr;
    ItclCmdsInfo *iciPtr;
    int checkMemoryLeaks;
    int i;
    int result;

    ItclShowArgs(1, "ItclFinishCmd", objc, objv);
    result = TCL_OK;
    infoPtr = Tcl_GetAssocData(interp, ITCL_INTERP_DATA, NULL);
    if (infoPtr == NULL) {
        infoPtr = (ItclObjectInfo *)clientData;
    }
    checkMemoryLeaks = 0;
    if (objc > 1) {
        if (strcmp(Tcl_GetString(objv[1]), "checkmemoryleaks") == 0) {
	    /* if we have that option, the namespace of the Tcl ensembles
	     * is not teared down, so we have to simulate it here to
	     * have the correct reference counts for infoPtr->infoVars2Ptr
	     * infoPtr->infoVars3Ptr and infoPtr->infoVars4Ptr
	     */
	    checkMemoryLeaks = 1;
	}
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * 2);
    newObjv[0] = Tcl_NewStringObj("my", -1);;
    for (i = 0; ;i++) {
        iciPtr = &itclCmds[i];
        if (iciPtr->name == NULL) {
	    break;
	}
	if ((iciPtr->flags & ITCL_IS_ENSEMBLE) == 0) {
            result = Itcl_RenameCommand(interp, iciPtr->name, "");
	} else {
	    objPtr = Tcl_NewStringObj(iciPtr->name, -1);
            newObjv[1] = objPtr;
	    Itcl_EnsembleDeleteCmd(infoPtr, infoPtr->interp, 2, newObjv);
	    Tcl_DecrRefCount(objPtr);
	}
        iciPtr++;
    }
    Tcl_DecrRefCount(newObjv[0]);
    ckfree((char *)newObjv);

    /* remove the unknown handler, to free the reference to the
     * Tcl_Obj with the name of it */
    ensObjPtr = Tcl_NewStringObj("::itcl::builtin::Info::delegated", -1);
    cmdPtr = Tcl_FindEnsemble(interp, ensObjPtr, TCL_LEAVE_ERR_MSG);
    if (cmdPtr != NULL) {
        Tcl_SetEnsembleUnknownHandler(NULL, cmdPtr, NULL);
    }
    Tcl_DecrRefCount(ensObjPtr);

    while (1) {
        hPtr = Tcl_FirstHashEntry(&infoPtr->instances, &place);
	if (hPtr == NULL) {
	    break;
	}
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(&infoPtr->instances);

    while (1) {
        hPtr = Tcl_FirstHashEntry(&infoPtr->classTypes, &place);
	if (hPtr == NULL) {
	    break;
	}
	Tcl_DeleteHashEntry(hPtr);
    }
    Tcl_DeleteHashTable(&infoPtr->classTypes);

    Tcl_DeleteHashTable(&infoPtr->procMethods);

    Tcl_DeleteHashTable(&infoPtr->objectCmds);
    Tcl_DeleteHashTable(&infoPtr->classes);
    Tcl_DeleteHashTable(&infoPtr->nameClasses);
    Tcl_DeleteHashTable(&infoPtr->namespaceClasses);

    nsPtr = Tcl_FindNamespace(interp, "::itcl::parser", NULL, 0);
    if (nsPtr != NULL) {
        Tcl_DeleteNamespace(nsPtr);
    }

    mapDict = NULL;
    ensObjPtr = Tcl_NewStringObj("::itcl::builtin::Info", -1);
    if (Tcl_FindNamespace(interp, Tcl_GetString(ensObjPtr), NULL, 0) != NULL) {
        Tcl_SetEnsembleUnknownHandler(NULL,
                Tcl_FindEnsemble(interp, ensObjPtr, TCL_LEAVE_ERR_MSG),
	        NULL);
    }
    Tcl_DecrRefCount(ensObjPtr);

    /* remove the vars entry from the info dict */
    /* and replace it by the original one */
    cmdPtr = Tcl_FindCommand(interp, "info", NULL, TCL_GLOBAL_ONLY);
    if (cmdPtr != NULL && Tcl_IsEnsemble(cmdPtr)) {
        Tcl_GetEnsembleMappingDict(NULL, cmdPtr, &mapDict);
        if (mapDict != NULL) {

	    objPtr = Tcl_NewStringObj("vars", -1);
	    Tcl_IncrRefCount(objPtr);
	    Tcl_DictObjRemove(interp, mapDict, objPtr);
	    Tcl_DictObjPut(interp, mapDict, objPtr, infoPtr->infoVars4Ptr);
	    Tcl_DecrRefCount(objPtr);
	    Tcl_SetEnsembleMappingDict(interp, cmdPtr, mapDict);
        }
    }
    /* FIXME have to figure out why the refCount of
     * ::itcl::builtin::Info
     * and ::itcl::builtin::Info::vars and vars is 2 here !! */
    /* seems to be as the tclOO commands are not yet deleted ?? */
    Tcl_DecrRefCount(infoPtr->infoVars3Ptr);
    Tcl_DecrRefCount(infoPtr->infoVars4Ptr);
    if (checkMemoryLeaks) {
        Tcl_DecrRefCount(infoPtr->infoVars3Ptr);
        Tcl_DecrRefCount(infoPtr->infoVars4Ptr);
    /* see comment above */
    }

    Tcl_DecrRefCount(infoPtr->typeDestructorArgumentPtr);

    Tcl_EvalEx(infoPtr->interp,
            "::oo::define ::itcl::clazz deletemethod unknown", -1, 0);

    /* first have to look for the remaining memory leaks, then remove the next ifdef */
    Itcl_RenameCommand(infoPtr->interp, "::itcl::clazz", "");

    /* tear down ::itcl namespace (this includes ::itcl::parser namespace) */
    nsPtr = Tcl_FindNamespace(infoPtr->interp, "::itcl::parser", NULL, 0);
    if (nsPtr != NULL) {
        Tcl_DeleteNamespace(nsPtr);
    }
    nsPtr = Tcl_FindNamespace(infoPtr->interp, "::itcl::import", NULL, 0);
    if (nsPtr != NULL) {
        Tcl_DeleteNamespace(nsPtr);
    }
    nsPtr = Tcl_FindNamespace(infoPtr->interp, "::itcl::internal", NULL, 0);
    if (nsPtr != NULL) {
        Tcl_DeleteNamespace(nsPtr);
    }
    nsPtr = Tcl_FindNamespace(infoPtr->interp, "::itcl::builtin", NULL, 0);
    if (nsPtr != NULL) {
        Tcl_DeleteNamespace(nsPtr);
    }
    nsPtr = Tcl_FindNamespace(infoPtr->interp, "::itcl", NULL, 0);
    if (nsPtr != NULL) {
        Tcl_DeleteNamespace(nsPtr);
    }

    /* cleanup ensemble info */
    ItclFinishEnsemble(infoPtr);

    ckfree((char *)infoPtr->class_meta_type);

    Itcl_DeleteStack(&infoPtr->clsStack);
    /* clean up list pool */
    Itcl_FinishList();

    Itcl_ReleaseData((ClientData)infoPtr);
    return result;
}

#ifdef OBJ_REF_COUNT_DEBUG
void Tcl_DbDumpRefCountInfo(const char *fileName, int noDeleted);


/*
 * ------------------------------------------------------------------------
 *  ItclDumpRefCountInfo()
 *
 *  debugging routine to check for memory leaks in use of Tcl_Obj's
 *
 * ------------------------------------------------------------------------
 */
static int
ItclDumpRefCountInfo(
    ClientData clientData,   /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int noDeleted;

    noDeleted = 0;
    if (objc > 1) {
        if (strcmp(Tcl_GetString(objv[1]), "-nodeleted") == 0) {
	    noDeleted = 1;
	}
    }
    ItclShowArgs(0, "ItclDumpRefCountInfo", objc, objv);
    Tcl_DbDumpRefCountInfo(NULL, noDeleted);
    return TCL_OK;
}
#endif

#ifdef ITCL_PRESERVE_DEBUG
void Itcl_DbDumpPreserveInfo(const char *fileName);


/*
 * ------------------------------------------------------------------------
 *  ItclDumpPreserveInfo()
 *
 *  debugging routine to check for memory leaks in use of Itcl_PreserveData
 *  and Itcl_ReleaseData
 *
 * ------------------------------------------------------------------------
 */
static int
ItclDumpPreserveInfo(
    ClientData clientData,   /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclShowArgs(0, "ItclDumpPreserveInfo", objc, objv);
    Itcl_DbDumpPreserveInfo(NULL);
    return TCL_OK;
}
#endif
