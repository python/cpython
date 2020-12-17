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
 *  Procedures in this file support the new syntax for [incr Tcl]
 *  class definitions:
 *
 *    itcl_class <className> {
 *        inherit <base-class>...
 *
 *        constructor {<arglist>} ?{<init>}? {<body>}
 *        destructor {<body>}
 *
 *        method <name> {<arglist>} {<body>}
 *        proc <name> {<arglist>} {<body>}
 *        variable <name> ?<init>? ?<config>?
 *        common <name> ?<init>?
 *
 *        public <thing> ?<args>...?
 *        protected <thing> ?<args>...?
 *        private <thing> ?<args>...?
 *    }
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

static const char initWidgetScript[] =
"namespace eval ::itcl {\n"
"    proc _find_widget_init {} {\n"
"        global env tcl_library\n"
"        variable library\n"
"        variable patchLevel\n"
"        rename _find_widget_init {}\n"
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
"            # On MacOSX, check the directories in the tcl_pkgPath\n"
"            if {[string equal $::tcl_platform(platform) \"unix\"] &&"
"                    [string equal $::tcl_platform(os) \"Darwin\"]} {\n"
"                foreach d $::tcl_pkgPath {\n"
"                    lappend dirs [file join $d itcl$patchLevel]\n"
"                }\n"
"            }\n"
"            # On *nix, check the directories in the tcl_pkgPath\n"
"            if {[string equal $::tcl_platform(platform) \"unix\"]} {\n"
"                foreach d $::tcl_pkgPath {\n"
"                    lappend dirs $d\n"
"                    lappend dirs [file join $d itcl$patchLevel]\n"
"                }\n"
"            }\n"
"        }\n"
"        foreach i $dirs {\n"
"            set library $i\n"
"            set itclfile [file join $i itclWidget.tcl]\n"
"            if {![catch {uplevel #0 [list source $itclfile]} emsg]} {\n"
"                return\n"
"            }\n"
"        }\n"
"        set msg \"Can't find a usable itclWidget.tcl in the following directories:\n\"\n"
"        append msg \"    $dirs\n\"\n"
"        append msg \"Last error:\n\"\n"
"        append msg \"    $emsg\n\"\n"
"        append msg \"This probably means that Itcl/Tcl weren't installed properly.\n\"\n"
"        append msg \"If you know where the Itcl library directory was installed,\n\"\n"
"        append msg \"you can set the environment variable ITCL_LIBRARY to point\n\"\n"
"        append msg \"to the library directory.\n\"\n"
"        error $msg\n"
"    }\n"
"    _find_widget_init\n"
"}";

/*
 *  Info needed for public/protected/private commands:
 */
typedef struct ProtectionCmdInfo {
    int pLevel;               /* protection level */
    ItclObjectInfo *infoPtr;  /* info regarding all known objects */
} ProtectionCmdInfo;

/*
 *  FORWARD DECLARATIONS
 */
static Tcl_CmdDeleteProc ItclFreeParserCommandData;
static void ItclDelObjectInfo(char* cdata);
static int ItclInitClassCommon(Tcl_Interp *interp, ItclClass *iclsPtr,
        ItclVariable *ivPtr, const char *initStr);

static Tcl_ObjCmdProc Itcl_ClassTypeVariableCmd;
static Tcl_ObjCmdProc Itcl_ClassTypeMethodCmd;
static Tcl_ObjCmdProc Itcl_ClassFilterCmd;
static Tcl_ObjCmdProc Itcl_ClassMixinCmd;
static Tcl_ObjCmdProc Itcl_WidgetCmd;
static Tcl_ObjCmdProc Itcl_WidgetAdaptorCmd;
static Tcl_ObjCmdProc Itcl_ClassComponentCmd;
static Tcl_ObjCmdProc Itcl_ClassTypeComponentCmd;
static Tcl_ObjCmdProc Itcl_ClassDelegateMethodCmd;
static Tcl_ObjCmdProc Itcl_ClassDelegateOptionCmd;
static Tcl_ObjCmdProc Itcl_ClassDelegateTypeMethodCmd;
static Tcl_ObjCmdProc Itcl_ClassForwardCmd;
static Tcl_ObjCmdProc Itcl_ClassMethodVariableCmd;
static Tcl_ObjCmdProc Itcl_ClassTypeConstructorCmd;
static Tcl_ObjCmdProc ItclGenericClassCmd;

static const struct {
    const char *name;
    Tcl_ObjCmdProc *objProc;
} parseCmds[] = {
    {"common", Itcl_ClassCommonCmd},
    {"component", Itcl_ClassComponentCmd},
    {"constructor", Itcl_ClassConstructorCmd},
    {"destructor", Itcl_ClassDestructorCmd},
    {"filter", Itcl_ClassFilterCmd},
    {"forward", Itcl_ClassForwardCmd},
    {"handleClass", Itcl_HandleClass},
    {"hulltype", Itcl_ClassHullTypeCmd},
    {"inherit", Itcl_ClassInheritCmd},
    {"method", Itcl_ClassMethodCmd},
    {"methodvariable", Itcl_ClassMethodVariableCmd},
    {"mixin", Itcl_ClassMixinCmd},
    {"option", Itcl_ClassOptionCmd},
    {"proc", Itcl_ClassProcCmd},
    {"typecomponent", Itcl_ClassTypeComponentCmd },
    {"typeconstructor", Itcl_ClassTypeConstructorCmd},
    {"typemethod", Itcl_ClassTypeMethodCmd},
    {"typevariable", Itcl_ClassTypeVariableCmd},
    {"variable", Itcl_ClassVariableCmd},
    {"widgetclass", Itcl_ClassWidgetClassCmd},
    {NULL, NULL}
};

static const struct {
    const char *name;
    Tcl_ObjCmdProc *objProc;
    int protection;
} protectionCmds[] = {
    {"private", Itcl_ClassProtectionCmd, ITCL_PRIVATE},
    {"protected", Itcl_ClassProtectionCmd, ITCL_PROTECTED},
    {"public", Itcl_ClassProtectionCmd, ITCL_PUBLIC},
    {NULL, NULL, 0}
};

/*
 * ------------------------------------------------------------------------
 *  Itcl_ParseInit()
 *
 *  Invoked by Itcl_Init() whenever a new interpeter is created to add
 *  [incr Tcl] facilities.  Adds the commands needed to parse class
 *  definitions.
 * ------------------------------------------------------------------------
 */
int
Itcl_ParseInit(
    Tcl_Interp *interp,     /* interpreter to be updated */
    ItclObjectInfo *infoPtr) /* info regarding all known objects and classes */
{
    Tcl_Namespace *parserNs;
    ProtectionCmdInfo *pInfoPtr;
    Tcl_DString buffer;
    int i;

    /*
     *  Create the "itcl::parser" namespace used to parse class
     *  definitions.
     */
    parserNs = Tcl_CreateNamespace(interp, "::itcl::parser",
        infoPtr, Itcl_ReleaseData);

    if (!parserNs) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            " (cannot initialize itcl parser)",
            NULL);
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    /*
     *  Add commands for parsing class definitions.
     */
    Tcl_DStringInit(&buffer);
    for (i=0 ; parseCmds[i].name ; i++) {
        Tcl_DStringAppend(&buffer, "::itcl::parser::", 16);
        Tcl_DStringAppend(&buffer, parseCmds[i].name, -1);
        Tcl_CreateObjCommand(interp, Tcl_DStringValue(&buffer),
                parseCmds[i].objProc, infoPtr, NULL);
        Tcl_DStringFree(&buffer);
    }

    for (i=0 ; protectionCmds[i].name ; i++) {
        Tcl_DStringAppend(&buffer, "::itcl::parser::", 16);
        Tcl_DStringAppend(&buffer, protectionCmds[i].name, -1);
        pInfoPtr = (ProtectionCmdInfo*)ckalloc(sizeof(ProtectionCmdInfo));
        pInfoPtr->pLevel = protectionCmds[i].protection;
        pInfoPtr->infoPtr = infoPtr;
        Tcl_CreateObjCommand(interp, Tcl_DStringValue(&buffer),
                protectionCmds[i].objProc, pInfoPtr,
		(Tcl_CmdDeleteProc*) ItclFreeParserCommandData);
        Tcl_DStringFree(&buffer);
    }

    /*
     *  Set the runtime variable resolver for the parser namespace,
     *  to control access to "common" data members while parsing
     *  the class definition.
     */
    if (infoPtr->useOldResolvers) {
        ItclSetParserResolver(parserNs);
    }
    /*
     *  Install the "class" command for defining new classes.
     */
    Tcl_CreateObjCommand(interp, "::itcl::class", Itcl_ClassCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::body", Itcl_BodyCmd,
        NULL, NULL);

    Tcl_CreateObjCommand(interp, "::itcl::configbody", Itcl_ConfigBodyCmd,
        NULL, NULL);

    Itcl_EventuallyFree(infoPtr, (Tcl_FreeProc *) ItclDelObjectInfo);

    /*
     *  Create the "itcl::find" command for high-level queries.
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::find") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::find",
            "classes", "?pattern?",
            Itcl_FindClassesCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::find",
            "objects", "?-class className? ?-isa className? ?pattern?",
            Itcl_FindObjectsCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);


    /*
     *  Create the "itcl::delete" command to delete objects
     *  and classes.
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::delete") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::delete",
            "class", "name ?name...?",
            Itcl_DelClassCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::delete",
            "object", "name ?name...?",
            Itcl_DelObjectCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::delete",
            "ensemble", "name ?name...?",
            Itcl_EnsembleDeleteCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    /*
     *  Create the "itcl::is" command to test object
     *  and classes existence.
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::is") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::is",
            "class", "name", Itcl_IsClassCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::is",
            "object", "?-class classname? name", Itcl_IsObjectCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);


    /*
     *  Add "code" and "scope" commands for handling scoped values.
     */
    Tcl_CreateObjCommand(interp, "::itcl::code", Itcl_CodeCmd,
        NULL, NULL);

    Tcl_CreateObjCommand(interp, "::itcl::scope", Itcl_ScopeCmd,
        NULL, NULL);

    /*
     *  Add the "filter" commands (add/delete)
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::filter") != TCL_OK) {
        return TCL_ERROR;
    }
    if (Itcl_AddEnsemblePart(interp, "::itcl::filter",
            "add", "objectOrClass filter ? ... ?", Itcl_FilterAddCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::filter",
            "delete", "objectOrClass filter ? ... ?", Itcl_FilterDeleteCmd,
            infoPtr, Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }

    Itcl_PreserveData(infoPtr);

    /*
     *  Add the "forward" commands (add/delete)
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::forward") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::forward",
            "add", "objectOrClass srcCommand targetCommand ? options ... ?",
	    Itcl_ForwardAddCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::forward",
            "delete", "objectOrClass targetCommand ? ... ?",
	    Itcl_ForwardDeleteCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    /*
     *  Add the "mixin" (add/delete) commands.
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::mixin") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::mixin",
            "add", "objectOrClass class ? class ... ?",
	    Itcl_MixinAddCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::mixin",
            "delete", "objectOrClass class ? class ... ?",
	    Itcl_MixinDeleteCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    /*
     *  Add commands for handling import stubs at the Tcl level.
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::import::stub") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::import::stub",
            "create", "name", Itcl_StubCreateCmd,
            NULL, NULL) != TCL_OK) {
        return TCL_ERROR;
    }
    if (Itcl_AddEnsemblePart(interp, "::itcl::import::stub",
            "exists", "name", Itcl_StubExistsCmd,
            NULL, NULL) != TCL_OK) {
        return TCL_ERROR;
    }

    Tcl_CreateObjCommand(interp, "::itcl::type", Itcl_TypeClassCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::widget", Itcl_WidgetCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::widgetadaptor", Itcl_WidgetAdaptorCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::nwidget", Itcl_NWidgetCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::addoption", Itcl_AddOptionCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::addobjectoption",
        Itcl_AddObjectOptionCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::adddelegatedoption",
        Itcl_AddDelegatedOptionCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::adddelegatedmethod",
        Itcl_AddDelegatedFunctionCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::addcomponent", Itcl_AddComponentCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::setcomponent", Itcl_SetComponentCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, "::itcl::extendedclass", Itcl_ExtendedClassCmd,
        infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    Tcl_CreateObjCommand(interp, ITCL_COMMANDS_NAMESPACE "::genericclass",
        ItclGenericClassCmd, infoPtr, Itcl_ReleaseData);
    Itcl_PreserveData(infoPtr);

    /*
     *  Add the "delegate" (method/option) commands.
     */
    if (Itcl_CreateEnsemble(interp, "::itcl::parser::delegate") != TCL_OK) {
        return TCL_ERROR;
    }

    if (Itcl_AddEnsemblePart(interp, "::itcl::parser::delegate",
            "method", "name to targetName as scipt using script",
	    Itcl_ClassDelegateMethodCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::parser::delegate",
            "typemethod", "name to targetName as scipt using script",
	    Itcl_ClassDelegateTypeMethodCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    if (Itcl_AddEnsemblePart(interp, "::itcl::parser::delegate",
            "option", "option to targetOption as script",
	    Itcl_ClassDelegateOptionCmd, infoPtr,
	    Itcl_ReleaseData) != TCL_OK) {
        return TCL_ERROR;
    }
    Itcl_PreserveData(infoPtr);

    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassCmd()
 *
 *  Invoked by Tcl whenever the user issues an "itcl::class" command to
 *  specify a class definition.  Handles the following syntax:
 *
 *    itcl::class <className> {
 *        inherit <base-class>...
 *
 *        constructor {<arglist>} ?{<init>}? {<body>}
 *        destructor {<body>}
 *
 *        method <name> {<arglist>} {<body>}
 *        proc <name> {<arglist>} {<body>}
 *        variable <varname> ?<init>? ?<config>?
 *        common <varname> ?<init>?
 *
 *        public <args>...
 *        protected <args>...
 *        private <args>...
 *    }
 *
 * ------------------------------------------------------------------------
 */
static int
ItclGenericClassCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *namePtr;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclComponent *icPtr;
    const char *typeStr;
    int result;


    ItclShowArgs(1, "ItclGenericClassCmd", objc-1, objv);
    if (objc != 4) {
	Tcl_AppendResult(interp, "usage: genericclass <classtype> <classname> ",
	        "<body>", NULL);
        return TCL_ERROR;
    }
    infoPtr = (ItclObjectInfo *)clientData;
    typeStr = Tcl_GetString(objv[1]);
    hPtr = Tcl_FindHashEntry(&infoPtr->classTypes, (char *)objv[1]);
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "genericclass bad classtype \"", typeStr,
                "\"", NULL);
        return TCL_ERROR;
    }
    result = ItclClassBaseCmd(clientData, interp, PTR2INT(Tcl_GetHashValue(hPtr)),
            objc - 1, objv + 1, &iclsPtr);
    if (result != TCL_OK) {
        return result;
    }
    if (PTR2INT(Tcl_GetHashValue(hPtr)) == ITCL_WIDGETADAPTOR) {
        /* create the itcl_hull variable */
        namePtr = Tcl_NewStringObj("itcl_hull", -1);
        if (ItclCreateComponent(interp, iclsPtr, namePtr, ITCL_COMMON,
	        &icPtr) != TCL_OK) {
            return TCL_ERROR;
        }
        iclsPtr->numVariables++;
    }
    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, Tcl_GetString(iclsPtr->fullNamePtr), NULL);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassCmd()
 *
 *  Invoked by Tcl whenever the user issues an "itcl::class" command to
 *  specify a class definition.  Handles the following syntax:
 *
 *    itcl::class <className> {
 *        inherit <base-class>...
 *
 *        constructor {<arglist>} ?{<init>}? {<body>}
 *        destructor {<body>}
 *
 *        method <name> {<arglist>} {<body>}
 *        proc <name> {<arglist>} {<body>}
 *        variable <varname> ?<init>? ?<config>?
 *        common <varname> ?<init>?
 *
 *        public <args>...
 *        protected <args>...
 *        private <args>...
 *    }
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    return ItclClassBaseCmd(clientData, interp, ITCL_CLASS, objc, objv, NULL);
}

/*
 * ------------------------------------------------------------------------
 *  ItclClassBaseCmd()
 *
 * ------------------------------------------------------------------------
 */

static Tcl_MethodCallProc ObjCallProc;
static Tcl_MethodCallProc ArgCallProc;
static Tcl_CloneProc CloneProc;

static const Tcl_MethodType itclObjMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
    "itcl objv method",
    ObjCallProc,
    Itcl_ReleaseData,
    CloneProc
};

static const Tcl_MethodType itclArgMethodType = {
    TCL_OO_METHOD_VERSION_CURRENT,
    "itcl argv method",
    ArgCallProc,
    Itcl_ReleaseData,
    CloneProc
};

static int
CloneProc(
    Tcl_Interp *interp,
    ClientData original,
    ClientData *copyPtr)
{
    Itcl_PreserveData((ItclMemberFunc *)original);
    *copyPtr = original;
    return TCL_OK;
}

static int
CallAfterCallMethod(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    ClientData clientData = data[0];
    Tcl_ObjectContext context = (Tcl_ObjectContext)data[1];

    return ItclAfterCallMethod(clientData, interp, context, NULL, result);
}

static int
ObjCallProc(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    ItclMemberFunc *imPtr = (ItclMemberFunc *)clientData;

    if (TCL_ERROR == ItclCheckCallMethod(clientData, interp, context,
	    NULL, NULL)) {
	return TCL_ERROR;
    }

    Tcl_NRAddCallback(interp, CallAfterCallMethod, clientData, context,
	    NULL, NULL);

    if ((imPtr->flags & ITCL_COMMON) == 0) {
	return Itcl_ExecMethod(clientData, interp, objc-1, objv+1);
    } else {
	return Itcl_ExecProc(clientData, interp, objc-1, objv+1);
    }
}

static int
ArgCallProc(
    ClientData clientData,
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv)
{
    return TCL_ERROR;
}

int
ItclClassBaseCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int flags,               /* flags: ITCL_CLASS, ITCL_TYPE,
                              * ITCL_WIDGET or ITCL_WIDGETADAPTOR */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[],   /* argument objects */
    ItclClass **iclsPtrPtr)  /* for returning iclsPtr */
{
    Tcl_Obj *argumentPtr;
    Tcl_Obj *bodyPtr;
    FOREACH_HASH_DECLS;
    Tcl_HashEntry *hPtr2;
    Tcl_Namespace *parserNs, *ooNs;
    Tcl_CallFrame frame;
    ItclClass *iclsPtr;
    ItclVariable *ivPtr;
    ItclObjectInfo* infoPtr;
    char *className;
    int isNewEntry;
    int result;
    int noCleanup;
    ItclMemberFunc *imPtr;

    infoPtr = (ItclObjectInfo*)clientData;
    if (iclsPtrPtr != NULL) {
        *iclsPtrPtr = NULL;
    }
    if (objc != 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "name { definition }");
        return TCL_ERROR;
    }
    ItclShowArgs(1, "ItclClassBaseCmd", objc, objv);
    className = Tcl_GetString(objv[1]);

    noCleanup = 0;
    /*
     *  Find the namespace to use as a parser for the class definition.
     *  If for some reason it is destroyed, bail out here.
     */
    parserNs = Tcl_FindNamespace(interp, "::itcl::parser",
        NULL, TCL_LEAVE_ERR_MSG);

    if (parserNs == NULL) {
        Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                "\n    (while parsing class definition for \"%s\")",
                className));
        return TCL_ERROR;
    }

    /*
     *  Try to create the specified class and its namespace.
     */
    /* need the workaround with infoPtr->currClassFlags to keep the stubs
     * call interface compatible!
     */
    infoPtr->currClassFlags = flags;
    if (Itcl_CreateClass(interp, className, infoPtr, &iclsPtr) != TCL_OK) {
        infoPtr->currClassFlags = 0;
        return TCL_ERROR;
    }
    infoPtr->currClassFlags = 0;
    iclsPtr->flags = flags;

    /*
     *  Import the built-in commands from the itcl::builtin namespace.
     *  Do this before parsing the class definition, so methods/procs
     *  can override the built-in commands.
     */
    result = Tcl_Import(interp, iclsPtr->nsPtr, "::itcl::builtin::*",
        /* allowOverwrite */ 1);
    ooNs = Tcl_GetObjectNamespace(iclsPtr->oPtr);
    if ( result == TCL_OK && ooNs != iclsPtr->nsPtr) {
	result = Tcl_Import(interp, ooNs, "::itcl::builtin::*", 1);
    }

    if (result != TCL_OK) {
        Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                "\n    (while installing built-in commands for class \"%s\")",
                className));
        goto errorReturn;
    }

    /*
     *  Push this class onto the class definition stack so that it
     *  becomes the current context for all commands in the parser.
     *  Activate the parser and evaluate the class definition.
     */
    Itcl_PushStack(iclsPtr, &infoPtr->clsStack);

    result = Itcl_PushCallFrame(interp, &frame, parserNs,
        /* isProcCallFrame */ 0);

    Itcl_SetCallFrameResolver(interp, iclsPtr->resolvePtr);
    if (result == TCL_OK) {
        result = Tcl_EvalObjEx(interp, objv[2], 0);
        Itcl_PopCallFrame(interp);
    }
    Itcl_PopStack(&infoPtr->clsStack);

    noCleanup = 0;
    if (result != TCL_OK) {
	Tcl_Obj *options = Tcl_GetReturnOptions(interp, result);
	Tcl_Obj *key = Tcl_NewStringObj("-errorline", -1);
	Tcl_Obj *stackTrace = NULL;

	Tcl_IncrRefCount(key);
	Tcl_DictObjGet(NULL, options, key, &stackTrace);
	Tcl_DecrRefCount(key);
	if (stackTrace == NULL) {
	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		    "\n    error while parsing class \"%s\" body %s",
		    className, Tcl_GetString(objv[2])));
	    noCleanup = 1;
	} else {
	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
		    "\n    (class \"%s\" body line %s)",
		    className, Tcl_GetString(stackTrace)));
	}
	Tcl_DecrRefCount(options);
        result = TCL_ERROR;
        goto errorReturn;
    }

    if (Itcl_FirstListElem(&iclsPtr->bases) == NULL) {
	/* No [inherit]. Use default inheritance root. */
	Tcl_Obj *cmdPtr = Tcl_NewListObj(4, NULL);

	Tcl_ListObjAppendElement(NULL, cmdPtr,
		Tcl_NewStringObj("::oo::define", -1));
	Tcl_ListObjAppendElement(NULL, cmdPtr, iclsPtr->fullNamePtr);
	Tcl_ListObjAppendElement(NULL, cmdPtr,
		Tcl_NewStringObj("superclass", -1));
	Tcl_ListObjAppendElement(NULL, cmdPtr,
		Tcl_NewStringObj("::itcl::Root", -1));

	Tcl_IncrRefCount(cmdPtr);
	result = Tcl_EvalObjEx(interp, cmdPtr, 0);
	Tcl_DecrRefCount(cmdPtr);
	if (result == TCL_ERROR) {
	    goto errorReturn;
	}
    }

    /*
     *  At this point, parsing of the class definition has succeeded.
     *  Add built-in methods such as "configure" and "cget"--as long
     *  as they don't conflict with those defined in the class.
     */
    if (Itcl_InstallBiMethods(interp, iclsPtr) != TCL_OK) {
        result = TCL_ERROR;
        goto errorReturn;
    }

    /*
     *  Build the name resolution tables for all data members.
     */
    Itcl_BuildVirtualTables(iclsPtr);

    /* make the methods and procs known to TclOO */
    FOREACH_HASH_VALUE(imPtr, &iclsPtr->functions) {
    	    ClientData pmPtr;
	    argumentPtr = imPtr->codePtr->argumentPtr;
	    bodyPtr = imPtr->codePtr->bodyPtr;

if (imPtr->codePtr->flags & ITCL_IMPLEMENT_OBJCMD) {
    /* Implementation of this member is coded in C expecting Tcl_Obj */

    imPtr->tmPtr = Tcl_NewMethod(interp, iclsPtr->clsPtr, imPtr->namePtr,
	    1, &itclObjMethodType, imPtr);
    Itcl_PreserveData(imPtr);

    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
	imPtr->tmPtr = Tcl_NewInstanceMethod(interp, iclsPtr->oPtr,
		imPtr->namePtr, 1, &itclObjMethodType, imPtr);
	Itcl_PreserveData(imPtr);
    }

} else if (imPtr->codePtr->flags & ITCL_IMPLEMENT_ARGCMD) {
    /* Implementation of this member is coded in C expecting (char *) */

    imPtr->tmPtr = Tcl_NewMethod(interp, iclsPtr->clsPtr, imPtr->namePtr,
	    1, &itclArgMethodType, imPtr);

		Itcl_PreserveData(imPtr);



} else {
	    if (imPtr->codePtr->flags & ITCL_BUILTIN) {
		int isDone;
		isDone = 0;
		if (imPtr->builtinArgumentPtr == NULL) {
/* FIXME next lines are possibly a MEMORY leak not really sure!! */
	            argumentPtr = Tcl_NewStringObj("args", -1);
		    imPtr->builtinArgumentPtr = argumentPtr;
		    Tcl_IncrRefCount(imPtr->builtinArgumentPtr);
		} else {
		    argumentPtr = imPtr->builtinArgumentPtr;
		}
	        bodyPtr = Tcl_NewStringObj("return [", -1);
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-cget") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::cget", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-configure") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::configure", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-isa") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::isa", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-createhull") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::createhull", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-keepcomponentoption") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::keepcomponentoption", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-ignorecomponentoption") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::ignorercomponentoption", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-renamecomponentoption") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::renamecomponentoption", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-keepoptioncomponent") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::keepoptioncomponent", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-ignoreoptioncomponent") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::ignoreoptioncomponent", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-renameoptioncomponent") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::renameoptioncomponent", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-setupcomponent") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::setupcomponent", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-initoptions") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::initoptions", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-getinstancevar") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::getinstancevar",
		            -1);
		    isDone = 1;
		}
		if (iclsPtr->flags &
		        (ITCL_TYPE|ITCL_WIDGETADAPTOR|
			ITCL_WIDGET|ITCL_ECLASS)) {
		/* now the builtin stuff for snit functionality */
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-mytypemethod") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::mytypemethod",
		            -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-mymethod") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::mymethod", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-myvar") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::myvar", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-mytypevar") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::mytypevar", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-itcl_hull") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::itcl_hull", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-callinstance") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::callinstance",
		            -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-myproc") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::myproc", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-installhull") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::installhull",
		            -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-installcomponent") == 0) {
		    Tcl_AppendToObj(bodyPtr,
		            "::itcl::builtin::installcomponent", -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-classunknown") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::classunknown",
		            -1);
		    isDone = 1;
		}
		if (strcmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-destroy") == 0) {
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::destroy", -1);
		    isDone = 1;
		}
		}
		if (strncmp(Tcl_GetString(imPtr->codePtr->bodyPtr),
		        "@itcl-builtin-setget", 20) == 0) {
		    char *cp = Tcl_GetString(imPtr->codePtr->bodyPtr)+20;
		    Tcl_AppendToObj(bodyPtr, "::itcl::builtin::setget ", -1);
		    Tcl_AppendToObj(bodyPtr, cp, -1);
		    Tcl_AppendToObj(bodyPtr, " ", 1);
		    isDone = 1;
		}
		if (!isDone) {
		    Tcl_AppendToObj(bodyPtr,
		        Tcl_GetString(imPtr->codePtr->bodyPtr), -1);
                }
	        Tcl_AppendToObj(bodyPtr, " {*}$args]", -1);
	    }
	    imPtr->tmPtr = Itcl_NewProcClassMethod(interp,
	        iclsPtr->clsPtr, ItclCheckCallMethod, ItclAfterCallMethod,
                ItclProcErrorProc, imPtr, imPtr->namePtr, argumentPtr,
		bodyPtr, &pmPtr);
	    hPtr2 = Tcl_CreateHashEntry(&iclsPtr->infoPtr->procMethods,
	            (char *)imPtr->tmPtr, &isNewEntry);
	    if (isNewEntry) {
	        Tcl_SetHashValue(hPtr2, imPtr);
	    }
	    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
		if (argumentPtr == NULL) {
		    argumentPtr = iclsPtr->infoPtr->typeDestructorArgumentPtr;
		    imPtr->codePtr->argumentPtr = argumentPtr;
		    Tcl_IncrRefCount(argumentPtr);
		}
		/*
		 * We're overwriting the tmPtr field, so yank out the
		 * entry in the procMethods map based on the old one.
		 */
		if (isNewEntry) {
		    Tcl_DeleteHashEntry(hPtr2);
		}
	        imPtr->tmPtr = Itcl_NewProcMethod(interp,
	            iclsPtr->oPtr, ItclCheckCallMethod, ItclAfterCallMethod,
                    ItclProcErrorProc, imPtr, imPtr->namePtr, argumentPtr,
		    bodyPtr, &pmPtr);
	    }
}
	    if ((imPtr->flags & ITCL_COMMON) == 0) {
	        imPtr->accessCmd = Tcl_CreateObjCommand(interp,
		        Tcl_GetString(imPtr->fullNamePtr),
		        Itcl_ExecMethod, imPtr, Itcl_ReleaseData);
		Itcl_PreserveData(imPtr);
	    } else {
	        imPtr->accessCmd = Tcl_CreateObjCommand(interp,
		        Tcl_GetString(imPtr->fullNamePtr),
			Itcl_ExecProc, imPtr, Itcl_ReleaseData);
		Itcl_PreserveData(imPtr);
	    }
    }
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	/* initialize the typecomponents and typevariables */
        if (Itcl_PushCallFrame(interp, &frame, iclsPtr->nsPtr,
                /*isProcCallFrame*/0) != TCL_OK) {
	    result = TCL_ERROR;
	    goto errorReturn;
        }
        FOREACH_HASH_VALUE(ivPtr, &iclsPtr->variables) {
	    if ((ivPtr->flags & ITCL_COMMON) && (ivPtr->init != NULL)) {
                if (Tcl_SetVar2(interp, Tcl_GetString(ivPtr->namePtr), NULL,
	                Tcl_GetString(ivPtr->init),
			TCL_NAMESPACE_ONLY) == NULL) {
                    Itcl_PopCallFrame(interp);
		    result = TCL_ERROR;
	            goto errorReturn;
                }
	    }
        }
        Itcl_PopCallFrame(interp);
    }
    if (iclsPtr->typeConstructorPtr != NULL) {
        /* call the typeconstructor body */
        if (Itcl_PushCallFrame(interp, &frame, iclsPtr->nsPtr,
                /*isProcCallFrame*/0) != TCL_OK) {
	    result = TCL_ERROR;
	    goto errorReturn;
        }
	result = Tcl_EvalObjEx(interp, iclsPtr->typeConstructorPtr,
	        TCL_EVAL_DIRECT);
        Itcl_PopCallFrame(interp);
        if (result != TCL_OK) {
	    goto errorReturn;
	}
    }
    result = TCL_OK;
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	if (ItclCheckForInitializedComponents(interp, iclsPtr, NULL) !=
	        TCL_OK) {
	    result = TCL_ERROR;
	    goto errorReturn;
	}
    }

    if (result == TCL_OK) {
        Tcl_ResetResult(interp);
    }
    if (iclsPtrPtr != NULL) {
        *iclsPtrPtr = iclsPtr;
    }
    ItclAddClassesDictInfo(interp, iclsPtr);
    return result;
errorReturn:
    if (!noCleanup) {
        Tcl_DeleteNamespace(iclsPtr->nsPtr);
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclCheckForInitializedComponents()
 *
 *  check if all components for delegation exist and are initialized
 * ------------------------------------------------------------------------
 */
int
ItclCheckForInitializedComponents(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclObject *ioPtr)
{
    FOREACH_HASH_DECLS;
    Tcl_CallFrame frame;
    Tcl_DString buffer;
    ItclDelegatedFunction *idmPtr;
    int result;
    int doCheck;

    result = TCL_OK;
    /* check if the typecomponents are initialized */
    if (Itcl_PushCallFrame(interp, &frame, iclsPtr->nsPtr,
            /*isProcCallFrame*/0) != TCL_OK) {
        return TCL_ERROR;
    }
    idmPtr = NULL;
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
        const char *val;
        /* check here for delegated typemethods only
         * rest is done in ItclCreateObject
         */
	doCheck = 1;
	if (ioPtr == NULL) {
            if (!(idmPtr->flags & ITCL_TYPE_METHOD)) {
	        doCheck = 0;
	        ioPtr = iclsPtr->infoPtr->currIoPtr;
	    }
	}
	if (doCheck) {
	    if (idmPtr->icPtr != NULL) {
		if (idmPtr->icPtr->ivPtr->flags & ITCL_COMMON) {
		    Tcl_Obj *objPtr;
		    objPtr = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE, -1);
		    Tcl_AppendToObj(objPtr, (Tcl_GetObjectNamespace(
			    idmPtr->icPtr->ivPtr->iclsPtr->oPtr))->fullName,
			    -1);
		    Tcl_AppendToObj(objPtr, "::", -1);
		    Tcl_AppendToObj(objPtr, Tcl_GetString(
		            idmPtr->icPtr->ivPtr->namePtr), -1);
	            val = Tcl_GetVar2(interp, Tcl_GetString(objPtr), NULL, 0);
		    Tcl_DecrRefCount(objPtr);
		} else {
		    Tcl_DStringInit(&buffer);
		    Tcl_DStringAppend(&buffer,
		            Tcl_GetString(ioPtr->varNsNamePtr), -1);
		    Tcl_DStringAppend(&buffer,
		            Tcl_GetString(idmPtr->icPtr->ivPtr->fullNamePtr),
			    -1);
	            val = Tcl_GetVar2(interp, Tcl_DStringValue(&buffer),
		            NULL, 0);
		    Tcl_DStringFree(&buffer);
		}
		if ((ioPtr != NULL) && ((val != NULL) && (strlen(val) == 0))) {
		    val = ItclGetInstanceVar(
			    ioPtr->iclsPtr->interp,
			    "itcl_hull", NULL, ioPtr,
			    iclsPtr);
                }
	        if ((val == NULL) || (strlen(val) == 0)) {
	            if (iclsPtr->flags & ITCL_WIDGETADAPTOR) {
	                if (strcmp (Tcl_GetString(idmPtr->icPtr->namePtr),
			        "itcl_hull") == 0) {
		            /* maybe that will be initialized in constructor
			     * later on */
	                    continue;
	                }
	            }
	            result = TCL_ERROR;
		    break;
	        }
	    }
	}
    }
    Itcl_PopCallFrame(interp);
    if (result == TCL_ERROR) {
        const char *startStr;
        const char *sepStr;
	const char *objectStr;
        startStr = "";
	sepStr = "";
	objectStr = "";
	if (ioPtr != NULL) {
	    sepStr = " ";
	    objectStr = Tcl_GetString(ioPtr->origNamePtr);
	}
        if (idmPtr->flags & ITCL_TYPE_METHOD) {
            startStr = "type";
        }
	/* FIXME there somtimes is a message for widgetadaptor:
	 * can't read "itcl_hull": no such variable
	 * have to check why
	 */
	Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, Tcl_GetString(iclsPtr->fullNamePtr),
		sepStr, objectStr, " delegates ", startStr, "method \"",
	        Tcl_GetString(idmPtr->namePtr),
	        "\" to undefined ", startStr, "component \"",
	        Tcl_GetString(idmPtr->icPtr->ivPtr->namePtr), "\"", NULL);
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassInheritCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "inherit" command is invoked to define one or more base classes.
 *  Handles the following syntax:
 *
 *      inherit <baseclass> ?<baseclass>...?
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassInheritCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    int result;
    int i;
    int newEntry;
    int haveClasses;
    const char *token;
    Itcl_ListElem *elem;
    Itcl_ListElem *elem2;
    ItclClass *cdPtr;
    ItclClass *baseClsPtr;
    ItclClass *badCdPtr;
    ItclHierIter hier;
    Itcl_Stack stack;
    Tcl_CallFrame frame;
    Tcl_DString buffer;

    ItclShowArgs(2, "Itcl_InheritCmd", objc, objv);

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "class ?class...?");
        return TCL_ERROR;
    }

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::inherit called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    /*
     *  An "inherit" statement can only be included once in a
     *  class definition.
     */
    elem = Itcl_FirstListElem(&iclsPtr->bases);
    if (elem != NULL) {
        Tcl_AppendToObj(Tcl_GetObjResult(interp), "inheritance \"", -1);

        while (elem) {
            cdPtr = (ItclClass*)Itcl_GetListValue(elem);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                Tcl_GetString(cdPtr->namePtr), " ", NULL);

            elem = Itcl_NextListElem(elem);
        }

        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "\" already defined for class \"",
	    Tcl_GetString(iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  Validate each base class and add it to the "bases" list.
     */
    result = Itcl_PushCallFrame(interp, &frame, iclsPtr->nsPtr->parentPtr,
        /* isProcCallFrame */ 0);

    if (result != TCL_OK) {
        return TCL_ERROR;
    }

    for (objc--,objv++; objc > 0; objc--,objv++) {

        /*
         *  Make sure that the base class name is known in the
         *  parent namespace (currently active).  If not, try
         *  to autoload its definition.
         */
        token = Tcl_GetString(*objv);
        baseClsPtr = Itcl_FindClass(interp, token, /* autoload */ 1);
        if (!baseClsPtr) {
            Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);
            int errlen;
            char *errmsg;

            Tcl_IncrRefCount(resultPtr);
            errmsg = Tcl_GetStringFromObj(resultPtr, &errlen);

            Tcl_ResetResult(interp);
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "cannot inherit from \"", token, "\"",
                NULL);

            if (errlen > 0) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    " (", errmsg, ")", NULL);
            }
            Tcl_DecrRefCount(resultPtr);
            goto inheritError;
        }

        /*
         *  Make sure that the base class is not the same as the
         *  class that is being built.
         */
        if (baseClsPtr == iclsPtr) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "class \"", Tcl_GetString(iclsPtr->namePtr),
		"\" cannot inherit from itself",
                NULL);
            goto inheritError;
        }

        Itcl_AppendList(&iclsPtr->bases, baseClsPtr);
	ItclPreserveClass(baseClsPtr);
    }

    /*
     *  Scan through the inheritance list to make sure that no
     *  class appears twice.
     */
    elem = Itcl_FirstListElem(&iclsPtr->bases);
    while (elem) {
        elem2 = Itcl_NextListElem(elem);
        while (elem2) {
            if (Itcl_GetListValue(elem) == Itcl_GetListValue(elem2)) {
                cdPtr = (ItclClass*)Itcl_GetListValue(elem);
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "class \"", iclsPtr->fullNamePtr,
                    "\" cannot inherit base class \"",
                    cdPtr->fullNamePtr, "\" more than once",
                    NULL);
                goto inheritError;
            }
            elem2 = Itcl_NextListElem(elem2);
        }
        elem = Itcl_NextListElem(elem);
    }

    /*
     *  Add each base class and all of its base classes into
     *  the heritage for the current class.  Along the way, make
     *  sure that no class appears twice in the heritage.
     */
    Itcl_InitHierIter(&hier, iclsPtr);
    cdPtr = Itcl_AdvanceHierIter(&hier);  /* skip the class itself */
    cdPtr = Itcl_AdvanceHierIter(&hier);
    while (cdPtr != NULL) {
        (void) Tcl_CreateHashEntry(&iclsPtr->heritage,
            (char*)cdPtr, &newEntry);

        if (!newEntry) {
            break;
        }
        cdPtr = Itcl_AdvanceHierIter(&hier);
    }
    Itcl_DeleteHierIter(&hier);

    /*
     *  Same base class found twice in the hierarchy?
     *  Then flag error.  Show the list of multiple paths
     *  leading to the same base class.
     */
    if (!newEntry) {
        Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);

        badCdPtr = cdPtr;
        Tcl_AppendStringsToObj(resultPtr,
            "class \"", Tcl_GetString(iclsPtr->fullNamePtr),
	    "\" inherits base class \"",
            Tcl_GetString(badCdPtr->fullNamePtr), "\" more than once:",
            NULL);

        cdPtr = iclsPtr;
        Itcl_InitStack(&stack);
        Itcl_PushStack(cdPtr, &stack);

        /*
         *  Show paths leading to bad base class
         */
        while (Itcl_GetStackSize(&stack) > 0) {
            cdPtr = (ItclClass*)Itcl_PopStack(&stack);

            if (cdPtr == badCdPtr) {
                Tcl_AppendToObj(resultPtr, "\n  ", -1);
                for (i=0; i < Itcl_GetStackSize(&stack); i++) {
                    if (Itcl_GetStackValue(&stack, i) == NULL) {
                        cdPtr = (ItclClass*)Itcl_GetStackValue(&stack, i-1);
                        Tcl_AppendStringsToObj(resultPtr,
                            Tcl_GetString(cdPtr->namePtr), "->",
                            NULL);
                    }
                }
                Tcl_AppendToObj(resultPtr,
		        Tcl_GetString(badCdPtr->namePtr), -1);
            }
            else if (!cdPtr) {
                (void)Itcl_PopStack(&stack);
            }
            else {
                elem = Itcl_LastListElem(&cdPtr->bases);
                if (elem) {
                    Itcl_PushStack(cdPtr, &stack);
                    Itcl_PushStack(NULL, &stack);
                    while (elem) {
                        Itcl_PushStack(Itcl_GetListValue(elem), &stack);
                        elem = Itcl_PrevListElem(elem);
                    }
                }
            }
        }
        Itcl_DeleteStack(&stack);
        goto inheritError;
    }

    /*
     *  At this point, everything looks good.
     *  Finish the installation of the base classes.  Update
     *  each base class to recognize the current class as a
     *  derived class.
     */
    Tcl_DStringInit(&buffer);
    haveClasses = 0;
    elem = Itcl_FirstListElem(&iclsPtr->bases);
    Tcl_DStringAppend(&buffer, "::oo::define ", -1);
    Tcl_DStringAppend(&buffer, Tcl_GetString(iclsPtr->fullNamePtr), -1);
    Tcl_DStringAppend(&buffer, " superclass", -1);
    while (elem) {
        baseClsPtr = (ItclClass*)Itcl_GetListValue(elem);
        haveClasses++;
        Tcl_DStringAppend(&buffer, " ", -1);
        Tcl_DStringAppend(&buffer, Tcl_GetString(baseClsPtr->fullNamePtr), -1);

        Itcl_AppendList(&baseClsPtr->derived, iclsPtr);
	ItclPreserveClass(iclsPtr);

        elem = Itcl_NextListElem(elem);
    }
    Itcl_PopCallFrame(interp);
    if (haveClasses) {
        result = Tcl_EvalEx(interp, Tcl_DStringValue(&buffer), -1, 0);
    }
    Tcl_DStringFree(&buffer);

    Itcl_BuildVirtualTables(iclsPtr);

    return result;


    /*
     *  If the "inherit" list cannot be built properly, tear it
     *  down and return an error.
     */
inheritError:
    Itcl_PopCallFrame(interp);

    elem = Itcl_FirstListElem(&iclsPtr->bases);
    while (elem) {
	ItclReleaseClass( (ItclClass *)Itcl_GetListValue(elem) );
        elem = Itcl_DeleteListElem(elem);
    }
    return TCL_ERROR;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassProtectionCmd()
 *
 *  Invoked by Tcl whenever the user issues a protection setting
 *  command like "public" or "private".  Creates commands and
 *  variables, and assigns a protection level to them.  Protection
 *  levels are defined as follows:
 *
 *    public    => accessible from any namespace
 *    protected => accessible from selected namespaces
 *    private   => accessible only in the namespace where it was defined
 *
 *  Handles the following syntax:
 *
 *    public <command> ?<arg> <arg>...?
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassProtectionCmd(
    ClientData clientData,   /* protection level (public/protected/private) */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ProtectionCmdInfo *pInfo = (ProtectionCmdInfo*)clientData;
    int result;
    int oldLevel;

    ItclShowArgs(2, "Itcl_ClassProtectionCmd", objc, objv);

    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "command ?arg arg...?");
        return TCL_ERROR;
    }

    oldLevel = Itcl_Protection(interp, pInfo->pLevel);

    if (objc == 2) {
	/* something like: public { variable a; variable b } */
        result = Tcl_EvalObjEx(interp, objv[1], 0);
    } else {
	/* something like: public variable a 123 456 */
        result = Itcl_EvalArgs(interp, objc-1, objv+1);
    }

    if (result == TCL_BREAK) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("invoked \"break\" outside of a loop",
                -1));
        result = TCL_ERROR;
    } else {
        if (result == TCL_CONTINUE) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("invoked \"continue\" outside of a loop",
                    -1));
            result = TCL_ERROR;
        } else {
	    if (result != TCL_OK) {
	        Tcl_Obj *options = Tcl_GetReturnOptions(interp, result);
	        Tcl_Obj *key = Tcl_NewStringObj("-errorline", -1);
	        Tcl_Obj *stackTrace = NULL;

	        Tcl_IncrRefCount(key);
	        Tcl_DictObjGet(NULL, options, key, &stackTrace);
	        Tcl_DecrRefCount(key);
	        if (stackTrace == NULL) {
                Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                    "\n    error while parsing class \"%s\"",
                    Tcl_GetString(objv[0])));
		} else {
                    char *token = Tcl_GetString(objv[0]);
                    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
                        "\n    (%.100s body line %s)",
                        token, Tcl_GetString(stackTrace)));
		}
            }
        }
    }

    Itcl_Protection(interp, oldLevel);
    return result;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassConstructorCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "constructor" command is invoked to define the constructor
 *  for an object.  Handles the following syntax:
 *
 *      constructor <arglist> ?<init>? <body>
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassConstructorCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    Tcl_Obj *namePtr;
    char *arglist;
    char *body;

    ItclShowArgs(2, "Itcl_ClassConstructorCmd", objc, objv);

    if (objc < 3 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "args ?init? body");
        return TCL_ERROR;
    }

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::constructor called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    namePtr = objv[0];
    if (Tcl_FindHashEntry(&iclsPtr->functions, (char *)objv[0])) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "\"", Tcl_GetString(namePtr), "\" already defined in class \"",
            Tcl_GetString(iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  If there is an object initialization statement, pick this
     *  out and take the last argument as the constructor body.
     */
    arglist = Tcl_GetString(objv[1]);
    if (objc == 3) {
        body = Tcl_GetString(objv[2]);
    } else {
        iclsPtr->initCode = objv[2];
        Tcl_IncrRefCount(iclsPtr->initCode);
        body = Tcl_GetString(objv[3]);
    }

    if (Itcl_CreateMethod(interp, iclsPtr, namePtr, arglist, body) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassDestructorCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "destructor" command is invoked to define the destructor
 *  for an object.  Handles the following syntax:
 *
 *      destructor <body>
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassDestructorCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    Tcl_Obj *namePtr;
    char *body;

    ItclShowArgs(2, "Itcl_ClassDestructorCmd", objc, objv);

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "body");
        return TCL_ERROR;
    }

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::destructor called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    namePtr = objv[0];
    body = Tcl_GetString(objv[1]);

    if (Tcl_FindHashEntry(&iclsPtr->functions, (char *)namePtr)) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "\"", Tcl_GetString(namePtr), "\" already defined in class \"",
            Tcl_GetString(iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    if (Itcl_CreateMethod(interp, iclsPtr, namePtr, NULL, body)
        != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassMethodCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "method" command is invoked to define an object method.
 *  Handles the following syntax:
 *
 *      method <name> ?<arglist>? ?<body>?
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassMethodCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *namePtr;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    char *arglist;
    char *body;

    ItclShowArgs(2, "Itcl_ClassMethodCmd", objc, objv);

    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "name ?args? ?body?");
        return TCL_ERROR;
    }

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::method called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    namePtr = objv[1];

    hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions, (char *)objv[1]);
    if (hPtr != NULL) {
	Tcl_AppendResult(interp, "method \"", Tcl_GetString(namePtr),
	        "\" has been delegated", NULL);
        return TCL_ERROR;
    }
    arglist = NULL;
    body = NULL;
    if (objc >= 3) {
        arglist = Tcl_GetString(objv[2]);
    }
    if (objc >= 4) {
        body = Tcl_GetString(objv[3]);
    }

    if (Itcl_CreateMethod(interp, iclsPtr, namePtr, arglist, body) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassProcCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "proc" command is invoked to define a common class proc.
 *  A "proc" is like a "method", but only has access to "common"
 *  class variables.  Handles the following syntax:
 *
 *      proc <name> ?<arglist>? ?<body>?
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassProcCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *namePtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    char *arglist;
    char *body;

    ItclShowArgs(1, "Itcl_ClassProcCmd", objc, objv);

    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "name ?args? ?body?");
        return TCL_ERROR;
    }

    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    namePtr = objv[1];

    arglist = NULL;
    body = NULL;
    if (objc >= 3) {
        arglist = Tcl_GetString(objv[2]);
    }
    if (objc >= 4) {
        body = Tcl_GetString(objv[3]);
    }

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::proc called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	const char *name = Tcl_GetString(namePtr);
        /* check if the typemethod is already delegated */
        FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	    if (strcmp(Tcl_GetString(idmPtr->namePtr), name) == 0) {
	        Tcl_AppendResult(interp, "Error in \"typemethod ", name,
		        "...\", \"", name, "\" has been delegated", NULL);
                return TCL_ERROR;
	    }
	}
    }
    if (Itcl_CreateProc(interp, iclsPtr, namePtr, arglist, body) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassTypeMethodCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "proc" command is invoked to define a common class proc.
 *  A "proc" is like a "method", but only has access to "common"
 *  class variables.  Handles the following syntax:
 *
 *      typemethod <name> ?<arglist>? ?<body>?
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassTypeMethodCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj *namePtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    char *arglist;
    char *body;
    ItclMemberFunc *imPtr;

    ItclShowArgs(1, "Itcl_ClassTypeMethodCmd", objc, objv);

    if (objc < 2 || objc > 4) {
        Tcl_WrongNumArgs(interp, 1, objv, "name ?args? ?body?");
        return TCL_ERROR;
    }

    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::typemethod called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    namePtr = objv[1];

    arglist = NULL;
    body = NULL;
    if (objc >= 3) {
        arglist = Tcl_GetString(objv[2]);
    }
    if (objc >= 4) {
        body = Tcl_GetString(objv[3]);
    }

    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
	const char *name = Tcl_GetString(namePtr);
        /* check if the typemethod is already delegated */
        FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
	    if (strcmp(Tcl_GetString(idmPtr->namePtr), name) == 0) {
	        Tcl_AppendResult(interp, "Error in \"typemethod ", name,
		        "...\", \"", name, "\" has been delegated", NULL);
                return TCL_ERROR;
	    }
	}
    }
    iclsPtr->infoPtr->functionFlags = ITCL_TYPE_METHOD;
    if (Itcl_CreateProc(interp, iclsPtr, namePtr, arglist, body) != TCL_OK) {
	iclsPtr->infoPtr->functionFlags = 0;
        return TCL_ERROR;
    }
    iclsPtr->infoPtr->functionFlags = 0;
    hPtr = Tcl_FindHashEntry(&iclsPtr->functions, (char *)namePtr);
    imPtr = (ItclMemberFunc *)Tcl_GetHashValue(hPtr);
    imPtr->flags |= ITCL_TYPE_METHOD;
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassVariableCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "variable" command is invoked to define an instance variable.
 *  Handles the following syntax:
 *
 *      variable <varname> ?<init>? ?<config>?
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassVariableCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *namePtr;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    ItclVariable *ivPtr;
    char *init;
    char *config;
    char *arrayInitStr;
    const char *usageStr;
    int pLevel;
    int haveError;
    int haveArrayInit;
    int result;

    result = TCL_OK;
    haveError = 0;
    haveArrayInit = 0;
    usageStr = NULL;
    arrayInitStr = NULL;
    ItclShowArgs(1, "Itcl_ClassVariableCmd", objc, objv);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::variable called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    pLevel = Itcl_Protection(interp, 0);
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
        if (objc > 2) {
	    if (strcmp(Tcl_GetString(objv[2]), "-array") == 0) {
	        if (objc == 4) {
		    arrayInitStr = Tcl_GetString(objv[3]);
		    haveArrayInit = 1;
		} else {
		    haveError = 1;
		    usageStr = "varname ?init|-array init?";
		}
	    }
	}
    }
    if (!haveError && !haveArrayInit) {
        if (pLevel == ITCL_PUBLIC) {
            if (objc < 2 || objc > 4) {
	        usageStr = "name ?init? ?config?";
	        haveError = 1;
            }
        } else {
            if ((objc < 2) || (objc > 3)) {
	        usageStr = "name ?init?";
	        haveError = 1;
            }
        }
    }

    if (haveError) {
        Tcl_WrongNumArgs(interp, 1, objv, usageStr);
        return TCL_ERROR;
    }
    /*
     *  Make sure that the variable name does not contain anything
     *  goofy like a "::" scope qualifier.
     */
    namePtr = objv[1];
    if (strstr(Tcl_GetString(namePtr), "::")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad variable name \"", Tcl_GetString(namePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    init   = NULL;
    config = NULL;
    if (!haveArrayInit) {
        if (objc >= 3) {
            init = Tcl_GetString(objv[2]);
        }
        if (objc >= 4) {
            config = Tcl_GetString(objv[3]);
        }
    }

    if (Itcl_CreateVariable(interp, iclsPtr, namePtr, init, config,
            &ivPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
        ivPtr->flags |= ITCL_VARIABLE;
    }
    if (haveArrayInit) {
        ivPtr->arrayInitPtr = Tcl_NewStringObj(arrayInitStr, -1);
        Tcl_IncrRefCount(ivPtr->arrayInitPtr);
    } else {
        ivPtr->arrayInitPtr = NULL;
    }
    iclsPtr->numVariables++;
    ItclAddClassVariableDictInfo(interp, iclsPtr, ivPtr);
    return result;
}


/*
 * ------------------------------------------------------------------------
 *  ItclInitClassCommon()
 *
 *  initialize a class commen variable
 *
 * ------------------------------------------------------------------------
 */
static int
ItclInitClassCommon(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    ItclVariable *ivPtr,
    const char *initStr)
{
    Tcl_DString buffer;
    Tcl_CallFrame frame;
    Tcl_Namespace *commonNsPtr;
    Tcl_HashEntry *hPtr;
    Tcl_Var varPtr;
    int result;
    int isNew;

    result = TCL_OK;
    ivPtr->flags |= ITCL_COMMON;
    iclsPtr->numCommons++;

    /*
     *  Create the variable in the namespace associated with the
     *  class.  Do this the hard way, to avoid the variable resolver
     *  procedures.  These procedures won't work until we rebuild
     *  the virtual tables below.
     */
    Tcl_DStringInit(&buffer);
    if (ivPtr->protection != ITCL_PUBLIC) {
        /* public commons go to the class namespace directly the others
	 * go to the variables namespace of the class */
        Tcl_DStringAppend(&buffer, ITCL_VARIABLES_NAMESPACE, -1);
    }
    Tcl_DStringAppend(&buffer,
	    (Tcl_GetObjectNamespace(ivPtr->iclsPtr->oPtr))->fullName, -1);
    commonNsPtr = Tcl_FindNamespace(interp, Tcl_DStringValue(&buffer), NULL, 0);
    if (commonNsPtr == NULL) {
        Tcl_AppendResult(interp, "ITCL: cannot find common variables namespace",
	        " for class \"", Tcl_GetString(ivPtr->iclsPtr->fullNamePtr),
		"\"", NULL);
	return TCL_ERROR;
    }
    varPtr = Tcl_NewNamespaceVar(interp, commonNsPtr,
            Tcl_GetString(ivPtr->namePtr));
    hPtr = Tcl_CreateHashEntry(&iclsPtr->classCommons, (char *)ivPtr,
            &isNew);
    if (isNew) {
	Itcl_PreserveVar(varPtr);
        Tcl_SetHashValue(hPtr, varPtr);
    }
    result = Itcl_PushCallFrame(interp, &frame, commonNsPtr,
        /* isProcCallFrame */ 0);
    Itcl_PopCallFrame(interp);

    /*
     *  If an initialization value was specified, then initialize
     *  the variable now, otherwise be sure the variable is uninitialized.
     */

    if (initStr != NULL) {
	const char *val;
	val = Tcl_SetVar2(interp, Tcl_GetString(ivPtr->fullNamePtr), NULL,
		initStr, TCL_NAMESPACE_ONLY);
        if (!val) {
            Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "cannot initialize common variable \"",
                Tcl_GetString(ivPtr->namePtr), "\"",
                NULL);
            return TCL_ERROR;
        }
    } else {
	/* previous var-lookup in class body (in ::itcl::parser) could obtain
	 * inherited common vars, so be sure it does not exists after new
	 * common creation (simply remove this reference). */
	Tcl_UnsetVar2(interp, Tcl_GetString(ivPtr->fullNamePtr), NULL,
		TCL_NAMESPACE_ONLY);
    }
    if (ivPtr->arrayInitPtr != NULL) {
	int i;
	int argc;
	const char **argv;
	const char *val;
	result = Tcl_SplitList(interp, Tcl_GetString(ivPtr->arrayInitPtr),
	        &argc, &argv);
	for (i = 0; i < argc; i++) {
            val = Tcl_SetVar2(interp, Tcl_GetString(ivPtr->fullNamePtr), argv[i],
                    argv[i + 1], TCL_NAMESPACE_ONLY);
            if (!val) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "cannot initialize common variable \"",
                    Tcl_GetString(ivPtr->namePtr), "\"",
                    NULL);
                return TCL_ERROR;
            }
	    i++;
        }
        ckfree((char *)argv);
    }
    Tcl_DStringFree(&buffer);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassCommonCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "common" command is invoked to define a variable that is
 *  common to all objects in the class.  Handles the following syntax:
 *
 *      common <varname> ?<init>?
 *
 * ------------------------------------------------------------------------
 */
static int
ItclClassCommonCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[],   /* argument objects */
    int protection,
    ItclVariable **ivPtrPtr)
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    ItclVariable *ivPtr;
    Tcl_Obj *namePtr;
    char *arrayInitStr;
    const char *usageStr;
    char *initStr;
    int haveError;
    int haveArrayInit;
    int result;

    result = TCL_OK;
    haveError = 0;
    haveArrayInit = 0;
    usageStr = NULL;
    arrayInitStr = NULL;
    *ivPtrPtr = NULL;
    ItclShowArgs(2, "Itcl_ClassCommonCmd", objc, objv);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::common called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGETADAPTOR)) {
        if (objc > 2) {
	    if (strcmp(Tcl_GetString(objv[2]), "-array") == 0) {
	        if (objc == 4) {
		    arrayInitStr = Tcl_GetString(objv[3]);
		    haveArrayInit = 1;
		} else {
		    haveError = 1;
		    usageStr = "varname ?init|-array init?";
		}
	    }
	}
    }
    if (!haveError && !haveArrayInit) {
        if ((objc < 2) || (objc > 3)) {
	    usageStr = "varname ?init?";
	    haveError = 1;
        }
    }
    if (haveError) {
        Tcl_WrongNumArgs(interp, 1, objv, usageStr);
        return TCL_ERROR;
    }
    /*
     *  Make sure that the variable name does not contain anything
     *  goofy like a "::" scope qualifier.
     */
    namePtr = objv[1];
    if (strstr(Tcl_GetString(namePtr), "::")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad variable name \"", Tcl_GetString(namePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    initStr = NULL;
    if (!haveArrayInit) {
        if (objc >= 3) {
            initStr = Tcl_GetString(objv[2]);
        }
    }

    if (Itcl_CreateVariable(interp, iclsPtr, namePtr, initStr, NULL,
            &ivPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (protection != 0) {
        ivPtr->protection = protection;
    }
    if (haveArrayInit) {
        ivPtr->arrayInitPtr = Tcl_NewStringObj(arrayInitStr, -1);
        Tcl_IncrRefCount(ivPtr->arrayInitPtr);
    } else {
        ivPtr->arrayInitPtr = NULL;
    }
    *ivPtrPtr = ivPtr;
    result =  ItclInitClassCommon(interp, iclsPtr, ivPtr, initStr);
    ItclAddClassVariableDictInfo(interp, iclsPtr, ivPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassTypeVariableCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "typevariable" command is invoked to define a variable that is
 *  common to all objects in the class.  Handles the following syntax:
 *
 *      typevariable <varname> ?<init>?
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassTypeVariableCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclVariable *ivPtr;
    int result;

    ivPtr = NULL;
    ItclShowArgs(1, "Itcl_ClassTypeVariableCmd", objc, objv);
    result = ItclClassCommonCmd(clientData, interp, objc, objv, ITCL_PUBLIC,
            &ivPtr);
    if (ivPtr != NULL) {
        ivPtr->flags |= ITCL_TYPE_VARIABLE;
        ItclAddClassVariableDictInfo(interp, ivPtr->iclsPtr, ivPtr);
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassCommonCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "common" command is invoked to define a variable that is
 *  common to all objects in the class.  Handles the following syntax:
 *
 *      common <varname> ?<init>?
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassCommonCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclVariable *ivPtr;

    ItclShowArgs(2, "Itcl_ClassTypeVariableCmd", objc, objv);
    return ItclClassCommonCmd(clientData, interp, objc, objv, 0, &ivPtr);
}


/*
 * ------------------------------------------------------------------------
 *  ItclFreeParserCommandData()
 *
 *  This callback will free() up memory dynamically allocated
 *  and passed as the ClientData argument to Tcl_CreateObjCommand.
 *  This callback is required because one can not simply pass
 *  a pointer to the free() or ckfree() to Tcl_CreateObjCommand.
 * ------------------------------------------------------------------------
 */
static void
ItclFreeParserCommandData(
    ClientData cdata)  /* client data to be destroyed */
{
    ckfree(cdata);
}

/*
 * ------------------------------------------------------------------------
 *  ItclDelObjectInfo()
 *
 *  Invoked when the management info for [incr Tcl] is no longer being
 *  used in an interpreter.  This will only occur when all class
 *  manipulation commands are removed from the interpreter.
 *
 *  See also FreeItclObjectInfo() in itclBase.c
 * ------------------------------------------------------------------------
 */
static void
ItclDelObjectInfo(
    char* cdata)    /* client data for class command */
{
    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)cdata;
    ItclObject *ioPtr;

    /*
     *  Destroy all known objects by deleting their access
     *  commands.
     */
    hPtr = Tcl_FirstHashEntry(&infoPtr->objects, &place);
    while (hPtr) {
        ioPtr = (ItclObject*)Tcl_GetHashValue(hPtr);
        Tcl_DeleteCommandFromToken(infoPtr->interp, ioPtr->accessCmd);
	    /*
	     * Fix 227804: Whenever an object to delete was found we
	     * have to reset the search to the beginning as the
	     * current entry in the search was deleted and accessing it
	     * is therefore not allowed anymore.
	     */

	    hPtr = Tcl_FirstHashEntry(&infoPtr->objects, &place);
	    /*hPtr = Tcl_NextHashEntry(&place);*/
    }
    Tcl_DeleteHashTable(&infoPtr->objects);
    Tcl_DeleteHashTable(&infoPtr->frameContext);

    Itcl_DeleteStack(&infoPtr->clsStack);
    Itcl_Free(infoPtr);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassFilterCmd()
 *
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassFilterCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj **newObjv;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    int result;

    ItclShowArgs(1, "Itcl_ClassFilterCmd", objc, objv);
    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::filter called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::widget/::itcl::widgetadaptor/::itcl::type",
		"/::itcl::extendedclass. Only these can have filters", NULL);
	return TCL_ERROR;
    }
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "<filterName> ?<filterName> ...?");
        return TCL_ERROR;
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+2));
    newObjv[0] = Tcl_NewStringObj("::oo::define", -1);
    Tcl_IncrRefCount(newObjv[0]);
    newObjv[1] = Tcl_NewStringObj(Tcl_GetString(iclsPtr->fullNamePtr), -1);
    Tcl_IncrRefCount(newObjv[1]);
    newObjv[2] = Tcl_NewStringObj("filter", -1);
    Tcl_IncrRefCount(newObjv[2]);
    memcpy(newObjv+3, objv+1, sizeof(Tcl_Obj *)*(objc-1));
ItclShowArgs(1, "Itcl_ClassFilterCmd2", objc+2, newObjv);
    result = Tcl_EvalObjv(interp, objc+2, newObjv, 0);
    Tcl_DecrRefCount(newObjv[0]);
    Tcl_DecrRefCount(newObjv[1]);
    Tcl_DecrRefCount(newObjv[2]);
    ckfree((char *)newObjv);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassMixinCmd()
 *
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassMixinCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclShowArgs(0, "Itcl_ClassMixinCmd", objc, objv);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_WidgetCmd()
 *
 *  that is just a dummy command to load package ItclWidget
 *  and then to resend the command and execute it in that package
 *  package ItclWidget is renaming the Tcl command!!
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_WidgetCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr;
    int result;

    ItclShowArgs(1, "Itcl_WidgetCmd", objc-1, objv);
    infoPtr = (ItclObjectInfo *)clientData;
    if (!infoPtr->itclWidgetInitted) {
        result =  Tcl_EvalEx(interp, initWidgetScript, -1, 0);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclWidgetInitted = 1;
    }
    return Tcl_EvalObjv(interp, objc, objv, 0);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_WidgetAdaptorCmd()
 *
 *  that is just a dummy command to load package ItclWidget
 *  and then to resend the command and execute it in that package
 *  package ItclWidget is renaming the Tcl command!!
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_WidgetAdaptorCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr;
    int result;

    ItclShowArgs(1, "Itcl_WidgetAdaptorCmd", objc-1, objv);
    infoPtr = (ItclObjectInfo *)clientData;
    if (!infoPtr->itclWidgetInitted) {
        result =  Tcl_EvalEx(interp, initWidgetScript, -1, 0);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclWidgetInitted = 1;
    }
    return Tcl_EvalObjv(interp, objc, objv, 0);
}

/*
 * ------------------------------------------------------------------------
 *  ItclParseOption()
 *
 *  Invoked by Tcl during the parsing whenever
 *  the "option" command is invoked to define an option
 *  Handles the following syntax:
 *
 *      option
 *
 * ------------------------------------------------------------------------
 */
int
ItclParseOption(
    ItclObjectInfo *infoPtr, /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[],   /* argument objects */
    ItclClass *iclsPtr,
    ItclObject *ioPtr,
    ItclOption **ioptPtrPtr) /* where the otpion info is found */
{
    Tcl_Obj *classNamePtr;
    Tcl_Obj *nameSpecPtr;
    Tcl_Obj **newObjv;
    Tcl_HashEntry *hPtr;
    ItclOption *ioptPtr;
    char *init;
    char *defaultValue;
    char *cgetMethod;
    char *cgetMethodVar;
    char *configureMethod;
    char *configureMethodVar;
    char *validateMethod;
    char *validateMethodVar;
    const char *token;
    const char *usage;
    const char *optionName;
    const char **argv;
    const char *name;
    const char *resourceName;
    const char *className;
    int argc;
    int pLevel;
    int readOnly;
    int newObjc;
    int foundOption;
    int result;
    int i;
    const char *cp;

    ItclShowArgs(1, "ItclParseOption", objc, objv);
    pLevel = Itcl_Protection(interp, 0);

    usage = "namespec \
?init? \
?-default value? \
?-readonly? \
?-cgetmethod methodName? \
?-cgetmethodvar varName? \
?-configuremethod methodName? \
?-configuremethodvar varName? \
?-validatemethod methodName? \
?-validatemethodvar varName";

    if (pLevel == ITCL_PUBLIC) {
        if (objc < 2 || objc > 11) {
            Tcl_WrongNumArgs(interp, 1, objv, usage);
            return TCL_ERROR;
        }
    } else {
        if ((objc < 2) || (objc > 12)) {
            Tcl_WrongNumArgs(interp, 1, objv, usage);
            return TCL_ERROR;
	}
    }

    argv = NULL;
    newObjv = NULL;
    defaultValue = NULL;
    cgetMethod = NULL;
    configureMethod = NULL;
    validateMethod = NULL;
    cgetMethodVar = NULL;
    configureMethodVar = NULL;
    validateMethodVar = NULL;
    readOnly = 0;
    newObjc = 0;
    optionName = Tcl_GetString(objv[1]);
    if (iclsPtr != NULL) {
        /* check for already delegated!! */
        hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedOptions, (char *)objv[1]);
	if (hPtr != NULL) {
	    Tcl_AppendResult(interp, "cannot define option \"", optionName,
	            "\" locally, it has already been delegated", NULL);
	    result = TCL_ERROR;
	    goto errorOut;
	}
    }
    if (ioPtr != NULL) {
        /* check for already delegated!! */
        hPtr = Tcl_FindHashEntry(&ioPtr->objectDelegatedOptions,
	        (char *)objv[1]);
	if (hPtr != NULL) {
	    Tcl_AppendResult(interp, "cannot define option \"", optionName,
	            "\" locally, it has already been delegated", NULL);
	    result = TCL_ERROR;
	    goto errorOut;
	}
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*objc);
    newObjv[newObjc] = objv[1];
    newObjc++;
    for (i=2; i<objc; i++) {
        token = Tcl_GetString(objv[i]);
	foundOption = 0;
	if (*token == '-') {
	    if (objc < i+1) {
                Tcl_WrongNumArgs(interp, 1, objv, usage);
	        result = TCL_ERROR;
	        goto errorOut;
	    }
	    if (strcmp(token, "-default") == 0) {
	        foundOption = 1;
		i++;
	        defaultValue = Tcl_GetString(objv[i]);
	    } else {
	      if (strcmp(token, "-readonly") == 0) {
	        foundOption = 1;
		readOnly = 1;
	      } else {
	        if (strncmp(token, "-cgetmethod", 11) == 0) {
	            if (strcmp(token, "-cgetmethod") == 0) {
	                foundOption = 1;
		        i++;
	                cgetMethod = Tcl_GetString(objv[i]);
		    }
	            if (strcmp(token, "-cgetmethodvar") == 0) {
	                foundOption = 1;
		        i++;
	                cgetMethodVar = Tcl_GetString(objv[i]);
		    }
		} else {
	          if (strncmp(token, "-configuremethod", 16) == 0) {
	              if (strcmp(token, "-configuremethod") == 0) {
	                  foundOption = 1;
		          i++;
	                  configureMethod = Tcl_GetString(objv[i]);
		      }
	              if (strcmp(token, "-configuremethodvar") == 0) {
	                  foundOption = 1;
		          i++;
	                  configureMethodVar = Tcl_GetString(objv[i]);
		      }
		  } else {
	            if (strncmp(token, "-validatemethod", 15) == 0) {
	                if (strcmp(token, "-validatemethod") == 0) {
	                    foundOption = 1;
		            i++;
	                    validateMethod = Tcl_GetString(objv[i]);
		        }
	                if (strcmp(token, "-validatemethodvar") == 0) {
	                    foundOption = 1;
		            i++;
	                    validateMethodVar = Tcl_GetString(objv[i]);
		        }
		    }
	          }
	        }
	      }
	    }
	    if (!foundOption) {
		Tcl_AppendResult(interp, "funny option command option: \"",
		    token, "\"", NULL);
	        return TCL_ERROR;
	    }
	}
	if (!foundOption) {
	    newObjv[newObjc] = objv[i];
	    newObjc++;
	}
    }

    if ((cgetMethod != NULL) && (cgetMethodVar != NULL)) {
        Tcl_AppendResult(interp,
	        "option -cgetmethod and -cgetmethodvar cannot be used both",
		NULL);
        result = TCL_ERROR;
        goto errorOut;
    }
    if ((configureMethod != NULL) && (configureMethodVar != NULL)) {
        Tcl_AppendResult(interp,
	        "option -configuremethod and -configuremethodvar",
		"cannot be used both",
		NULL);
        result = TCL_ERROR;
        goto errorOut;
    }
    if ((validateMethod != NULL) && (validateMethodVar != NULL)) {
        Tcl_AppendResult(interp,
	        "option -validatemethod and -validatemethodvar",
		"cannot be used both",
		NULL);
        result = TCL_ERROR;
        goto errorOut;
    }
    if (newObjc < 1) {
        Tcl_AppendResult(interp, "usage: option ", usage, NULL);
        result = TCL_ERROR;
        goto errorOut;
    }
    resourceName = NULL;
    className = NULL;

    nameSpecPtr = newObjv[0];
    token = Tcl_GetString(nameSpecPtr);
    if (Tcl_SplitList(interp, (const char *)token, &argc, &argv) != TCL_OK) {
        result = TCL_ERROR;
        goto errorOut;
    }
    name = argv[0];
    if (*name != '-') {
	Tcl_AppendResult(interp, "bad option name \"", name,
	        "\", options must start with a \"-\"", NULL);
        result = TCL_ERROR;
        goto errorOut;
    }

    /*
     *  Make sure that the variable name does not contain anything
     *  goofy like a "::" scope qualifier.
     */
    if (strstr(name, "::")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option name \"", name,
	        "\", option names must not contain \"::\"", NULL);
        result = TCL_ERROR;
        goto errorOut;
    }
    if (strstr(name, " ")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option name \"", name,
	        "\", option names must not contain \" \"", NULL);
        result = TCL_ERROR;
        goto errorOut;
    }
    cp = name;
    while (*cp) {
        if (isupper(UCHAR(*cp))) {
	    Tcl_AppendResult(interp, "bad option name \"", name, "\" ",
	            ", options must not contain uppercase characters", NULL);
            result = TCL_ERROR;
            goto errorOut;
	}
	cp++;
    }
    if (argc > 1) {
        resourceName = argv[1];
    } else {
	/* resource name defaults to option name minus hyphen */
        resourceName = name+1;
    }
    if (argc > 2) {
        className = argv[2];
    } else {
	/* class name defaults to option name minus hyphen and capitalized */
        className = resourceName;
    }
    classNamePtr = ItclCapitalize(className);
    init = defaultValue;
    if ((newObjc > 1) && (init == NULL)) {
        init = Tcl_GetString(newObjv[1]);
    }

    ioptPtr = (ItclOption*)Itcl_Alloc(sizeof(ItclOption));
    ioptPtr->protection   = Itcl_Protection(interp, 0);
    if (ioptPtr->protection == ITCL_DEFAULT_PROTECT) {
        ioptPtr->protection = ITCL_PROTECTED;
    }
    ioptPtr->namePtr      = Tcl_NewStringObj(name, -1);
    Tcl_IncrRefCount(ioptPtr->namePtr);
    ioptPtr->resourceNamePtr = Tcl_NewStringObj(resourceName, -1);
    Tcl_IncrRefCount(ioptPtr->resourceNamePtr);
    ioptPtr->classNamePtr = Tcl_NewStringObj(Tcl_GetString(classNamePtr), -1);
    Tcl_IncrRefCount(ioptPtr->classNamePtr);
    Tcl_DecrRefCount(classNamePtr);

    if (init) {
        ioptPtr->defaultValuePtr = Tcl_NewStringObj(init, -1);
        Tcl_IncrRefCount(ioptPtr->defaultValuePtr);
    }
    if (cgetMethod != NULL) {
        ioptPtr->cgetMethodPtr = Tcl_NewStringObj(cgetMethod, -1);
        Tcl_IncrRefCount(ioptPtr->cgetMethodPtr);
    }
    if (configureMethod != NULL) {
        ioptPtr->configureMethodPtr = Tcl_NewStringObj(configureMethod, -1);
        Tcl_IncrRefCount(ioptPtr->configureMethodPtr);
    }
    if (validateMethod != NULL) {
        ioptPtr->validateMethodPtr = Tcl_NewStringObj(validateMethod, -1);
        Tcl_IncrRefCount(ioptPtr->validateMethodPtr);
    }
    if (cgetMethodVar != NULL) {
        ioptPtr->cgetMethodVarPtr = Tcl_NewStringObj(cgetMethodVar, -1);
        Tcl_IncrRefCount(ioptPtr->cgetMethodVarPtr);
    }
    if (configureMethodVar != NULL) {
        ioptPtr->configureMethodVarPtr = Tcl_NewStringObj(configureMethodVar, -1);
        Tcl_IncrRefCount(ioptPtr->configureMethodVarPtr);
    }
    if (validateMethodVar != NULL) {
        ioptPtr->validateMethodVarPtr = Tcl_NewStringObj(validateMethodVar, -1);
        Tcl_IncrRefCount(ioptPtr->validateMethodVarPtr);
    }
    if (readOnly != 0) {
        ioptPtr->flags |= ITCL_OPTION_READONLY;
    }

    *ioptPtrPtr = ioptPtr;
    ItclAddOptionDictInfo(interp, iclsPtr, ioptPtr);
    result = TCL_OK;
errorOut:
    if (argv != NULL) {
        ckfree((char *)argv);
    }
    if (newObjv != NULL) {
        ckfree((char *)newObjv);
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassOptionCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "option" command is invoked to define an option
 *  Handles the following syntax:
 *
 *      option
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_ClassOptionCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclOption *ioptPtr;
    const char *tkPackage;
    const char *tkVersion;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);

    ItclShowArgs(1, "Itcl_ClassOptionCmd", objc, objv);

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::option called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "a \"class\" cannot have options", NULL);
	return TCL_ERROR;
    }

    if ((objc > 1) && (strcmp(Tcl_GetString(objv[1]), "add") == 0)) {
	tkVersion = "8.6";
        tkPackage = Tcl_PkgPresentEx(interp, "Tk", tkVersion, 0, NULL);
        if (tkPackage == NULL) {
	    tkPackage = Tcl_PkgRequireEx(interp, "Tk", tkVersion, 0, NULL);
	}
	if (tkPackage == NULL) {
	    Tcl_AppendResult(interp, "cannot load package Tk", tkVersion,
	            NULL);
	    return TCL_ERROR;
	}
        return Tcl_EvalObjv(interp, objc, objv, TCL_EVAL_GLOBAL);
    }
    if (ItclParseOption(infoPtr, interp, objc, objv, iclsPtr, NULL,
            &ioptPtr) != TCL_OK) {
	return TCL_ERROR;
    }

    if (Itcl_CreateOption(interp, iclsPtr, ioptPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclCreateComponent()
 *
 *
 * ------------------------------------------------------------------------
 */
int
ItclCreateComponent(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    Tcl_Obj *componentPtr,
    int type,
    ItclComponent **icPtrPtr)
{
    Tcl_HashEntry *hPtr;
    ItclComponent *icPtr;
    ItclVariable *ivPtr;
    int result;
    int isNew;

    if (iclsPtr == NULL) {
	return TCL_OK;
    }
    hPtr = Tcl_CreateHashEntry(&iclsPtr->components, (char *)componentPtr,
            &isNew);
    if (isNew) {
        if (Itcl_CreateVariable(interp, iclsPtr, componentPtr, NULL, NULL,
                &ivPtr) != TCL_OK) {
            return TCL_ERROR;
        }
	if (type & ITCL_COMMON) {
	    result = ItclInitClassCommon(interp, iclsPtr, ivPtr, "");
	    if (result != TCL_OK) {
	        return result;
	    }
	}
	if (iclsPtr->flags & (ITCL_WIDGET|ITCL_WIDGETADAPTOR)) {
	    if (strcmp(Tcl_GetString(componentPtr), "itcl_hull") == 0) {
	        /* special built in itcl_hull variable */
		ivPtr->initted = 1;
	        ivPtr->flags |= ITCL_HULL_VAR;
	    }
	}
        ivPtr->flags |= ITCL_COMPONENT_VAR;
        icPtr = (ItclComponent *)ckalloc(sizeof(ItclComponent));
        memset(icPtr, 0, sizeof(ItclComponent));
	Tcl_InitObjHashTable(&icPtr->keptOptions);
        icPtr->namePtr = componentPtr;
        Tcl_IncrRefCount(icPtr->namePtr);
        icPtr->ivPtr = ivPtr;
	Tcl_SetHashValue(hPtr, icPtr);
        ItclAddClassVariableDictInfo(interp, iclsPtr, ivPtr);
    } else {
        icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
    }
    *icPtrPtr = icPtr;
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclHandleClassComponent()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "component" command is invoked to define a component
 *  Handles the following syntax:
 *
 *      component
 *
 * ------------------------------------------------------------------------
 */
static int
ItclHandleClassComponent(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[],   /* argument objects */
    ItclComponent **icPtrPtr)
{
    Tcl_Obj **newObjv;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclComponent *icPtr;
    const char *usage;
    const char *publ;
    int inherit;
    int haveInherit;
    int havePublic;
    int newObjc;
    int haveValue;
    int storageClass;
    int i;

    ItclShowArgs(1, "Itcl_ClassComponentCmd", objc, objv);
    if (icPtrPtr != NULL) {
        *icPtrPtr = NULL;
    }
    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::component called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    usage = "component ?-public <typemethod>? ?-inherit ?<flag>??";
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::extendedclass/::itcl::widget",
		"/::itcl::widgetadaptor/::itcl::type.",
		" Only these can have components", NULL);
	return TCL_ERROR;
    }
    if ((objc < 2) && (objc > 6)) {
        Tcl_AppendResult(interp, "wrong # args should be: ", usage, NULL);
        return TCL_ERROR;
    }
    inherit = 0;
    haveInherit = 0;
    publ = NULL;
    havePublic = 0;
    for (i = 2; i < objc; i++) {
        if (strcmp(Tcl_GetString(objv[i]), "-inherit") == 0) {
	    if (haveInherit) {
                Tcl_AppendResult(interp, "wrong syntax should be: ",
		        usage, NULL);
                return TCL_ERROR;
	    }
	    haveValue = 0;
	    inherit = 1;
	    if (i < objc - 1) {
	        if (strcmp(Tcl_GetString(objv[i + 1]), "yes") == 0) {
		    haveValue = 1;
		}
	        if (strcmp(Tcl_GetString(objv[i + 1]), "YES") == 0) {
		    haveValue = 1;
		}
	        if (strcmp(Tcl_GetString(objv[i + 1]), "no") == 0) {
		    haveValue = 1;
		    inherit = 0;
		}
	        if (strcmp(Tcl_GetString(objv[i + 1]), "NO") == 0) {
		    haveValue = 1;
		    inherit = 0;
		}
	    }
	    if (haveValue) {
	        i++;
	    }
	    haveInherit = 1;
	} else {
            if (strcmp(Tcl_GetString(objv[i]), "-public") == 0) {
	        if (havePublic) {
                    Tcl_AppendResult(interp, "wrong syntax should be: ",
		            usage, NULL);
                    return TCL_ERROR;
	        }
	        havePublic = 1;
	        if (i >= objc - 1) {
                    Tcl_AppendResult(interp, "wrong syntax should be: ",
		            usage, NULL);
                    return TCL_ERROR;
		}
	        publ = Tcl_GetString(objv[i + 1]);
            } else {
                Tcl_AppendResult(interp, "wrong syntax should be: ",
		        usage, NULL);
                return TCL_ERROR;
	    }
	}
	i++;
    }
    storageClass = ITCL_COMMON;
    if (iclsPtr->flags & ITCL_ECLASS) {
        storageClass = 0;
    }
    if (ItclCreateComponent(interp, iclsPtr, objv[1], storageClass,
            &icPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (inherit) {
        icPtr->flags |= ITCL_COMPONENT_INHERIT;
	newObjc = 4;
	newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*)*newObjc);
	newObjv[0] = Tcl_NewStringObj("delegate::option", -1);
	Tcl_IncrRefCount(newObjv[0]);
	newObjv[1] = Tcl_NewStringObj("*", -1);
	Tcl_IncrRefCount(newObjv[1]);
	newObjv[2] = Tcl_NewStringObj("to", -1);
	Tcl_IncrRefCount(newObjv[2]);
	newObjv[3] = objv[1];
	Tcl_IncrRefCount(newObjv[3]);
        if (Itcl_ClassDelegateOptionCmd(infoPtr, interp, newObjc, newObjv)
	        != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_SetStringObj(newObjv[0] , "delegate::method", -1);
        if (Itcl_ClassDelegateMethodCmd(infoPtr, interp, newObjc, newObjv)
	        != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DecrRefCount(newObjv[0]);
	Tcl_DecrRefCount(newObjv[1]);
	Tcl_DecrRefCount(newObjv[2]);
	Tcl_DecrRefCount(newObjv[3]);
        ckfree((char *)newObjv);
    }
    if (publ != NULL) {
        icPtr->flags |= ITCL_COMPONENT_PUBLIC;
	newObjc = 4;
	newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*)*newObjc);
	newObjv[0] = Tcl_NewStringObj("delegate::method", -1);
	Tcl_IncrRefCount(newObjv[0]);
	newObjv[1] = Tcl_NewStringObj(publ, -1);
	Tcl_IncrRefCount(newObjv[1]);
	newObjv[2] = Tcl_NewStringObj("to", -1);
	Tcl_IncrRefCount(newObjv[2]);
	newObjv[3] = objv[1];
	Tcl_IncrRefCount(newObjv[3]);
        ItclShowArgs(1, "COMPPUB", newObjc, newObjv);
        if (Itcl_ClassDelegateMethodCmd(infoPtr, interp, newObjc, newObjv)
	        != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_DecrRefCount(newObjv[0]);
	Tcl_DecrRefCount(newObjv[1]);
	Tcl_DecrRefCount(newObjv[2]);
	Tcl_DecrRefCount(newObjv[3]);
        ckfree((char *)newObjv);
    }
    if (icPtrPtr != NULL) {
        *icPtrPtr = icPtr;
    }
    ItclAddClassComponentDictInfo(interp, iclsPtr, icPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassComponentCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "component" command is invoked to define a component
 *  Handles the following syntax:
 *
 *      component
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassComponentCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclComponent *icPtr;

    return ItclHandleClassComponent(clientData, interp, objc, objv, &icPtr);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassTypeComponentCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "typecomponent" command is invoked to define a typecomponent
 *  Handles the following syntax:
 *
 *      component
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassTypeComponentCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclComponent *icPtr;
    int result;

    ItclShowArgs(1, "Itcl_ClassTypeComponentCmd", objc, objv);
    result = ItclHandleClassComponent(clientData, interp, objc, objv, &icPtr);
    if (result != TCL_OK) {
       return result;
    }
    icPtr->ivPtr->flags |= ITCL_COMMON;
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  ItclCreateDelegatedFunction()
 *
 *  Install a delegated function for a class
 *
 * ------------------------------------------------------------------------
 */
int
ItclCreateDelegatedFunction(
    Tcl_Interp *interp,
    ItclClass *iclsPtr,
    Tcl_Obj *methodNamePtr,
    ItclComponent *icPtr,
    Tcl_Obj *targetPtr,
    Tcl_Obj *usingPtr,
    Tcl_Obj *exceptionsPtr,
    ItclDelegatedFunction **idmPtrPtr)
{
    ItclDelegatedFunction *idmPtr;
    const char **argv;
    int argc;
    int isNew;
    int i;

    idmPtr = (ItclDelegatedFunction *)ckalloc(sizeof(ItclDelegatedFunction));
    memset(idmPtr, 0, sizeof(ItclDelegatedFunction));
    Tcl_InitObjHashTable(&idmPtr->exceptions);
    idmPtr->namePtr = Tcl_NewStringObj(Tcl_GetString(methodNamePtr), -1);
    Tcl_IncrRefCount(idmPtr->namePtr);
    idmPtr->icPtr = icPtr;
    idmPtr->asPtr = targetPtr;
    if (idmPtr->asPtr != NULL) {
        Tcl_IncrRefCount(idmPtr->asPtr);
    }
    idmPtr->usingPtr = usingPtr;
    if (idmPtr->usingPtr != NULL) {
        Tcl_IncrRefCount(idmPtr->usingPtr);
    }
    if (exceptionsPtr != NULL) {
        if (Tcl_SplitList(interp, Tcl_GetString(exceptionsPtr), &argc, &argv)
	        != TCL_OK) {
	    return TCL_ERROR;
	}
        for(i = 0; i < argc; i++) {
	    Tcl_Obj *objPtr;
	    objPtr = Tcl_NewStringObj(argv[i], -1);
	    Tcl_CreateHashEntry(&idmPtr->exceptions, (char *)objPtr,
	            &isNew);
	}
	ckfree((char *) argv);
    }
    if (idmPtrPtr != NULL) {
        *idmPtrPtr = idmPtr;
    }
    ItclAddClassDelegatedFunctionDictInfo(interp, iclsPtr, idmPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_HandleDelegateMethodCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "delegate method" command is invoked to define a
 *  Handles the following syntax:
 *
 *      delegate method
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_HandleDelegateMethodCmd(
    Tcl_Interp *interp,      /* current interpreter */
    ItclObject *ioPtr,       /* != NULL for ::itcl::adddelegatedmethod
                                otherwise NULL */
    ItclClass *iclsPtr,      /* != NULL for delegate method otherwise NULL */
    ItclDelegatedFunction **idmPtrPtr,
                             /* where to return idoPtr */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *methodNamePtr;
    Tcl_Obj *componentPtr;
    Tcl_Obj *targetPtr;
    Tcl_Obj *usingPtr;
    Tcl_Obj *exceptionsPtr;
    Tcl_HashEntry *hPtr;
    ItclClass *iclsPtr2;
    ItclComponent *icPtr;
    ItclHierIter hier;
    const char *usageStr;
    const char *methodName;
    const char *component;
    const char *token;
    int result;
    int i;
    int foundOpt;

    ItclShowArgs(1, "Itcl_HandleDelegateMethodCmd", objc, objv);
    usageStr = "delegate method <methodName> to <componentName> ?as <targetName>?\n\
delegate method <methodName> ?to <componentName>? using <pattern>\n\
delegate method * ?to <componentName>? ?using <pattern>? ?except <methods>?";
    if (objc < 4) {
	Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
        return TCL_ERROR;
    }
    componentPtr = NULL;
    icPtr = NULL;
    methodName = Tcl_GetString(objv[1]);
    component = NULL;
    targetPtr = NULL;
    usingPtr = NULL;
    exceptionsPtr = NULL;
    for(i=2;i<objc;i++) {
        token = Tcl_GetString(objv[i]);
	if (i+1 == objc) {
	    Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
	    return TCL_ERROR;
	}
	foundOpt = 0;
	if (strcmp(token, "to") == 0) {
	    i++;
	    component = Tcl_GetString(objv[i]);
	    componentPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "as") == 0) {
	    i++;
	    targetPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "except") == 0) {
	    i++;
	    exceptionsPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "using") == 0) {
	    i++;
	    usingPtr = objv[i];
	    foundOpt++;
        }
        if (!foundOpt) {
	    Tcl_AppendResult(interp, "bad option \"", token, "\" should be ",
	            usageStr, NULL);
	    return TCL_ERROR;
	}
    }
    if ((exceptionsPtr != NULL) && (*methodName != '*')) {
	Tcl_AppendResult(interp,
	        "can only specify \"except\" with \"delegate method *\"", NULL);
	return TCL_ERROR;
    }
    if ((component == NULL) && (usingPtr == NULL)) {
	Tcl_AppendResult(interp, "missing to should be: ", usageStr, NULL);
	return TCL_ERROR;
    }
    if ((*methodName == '*') && (targetPtr != NULL)) {
	Tcl_AppendResult(interp,
	        "cannot specify \"as\" with \"delegate method *\"", NULL);
	return TCL_ERROR;
    }
    /* check for already delegated */
    methodNamePtr = Tcl_NewStringObj(methodName, -1);
    if (ioPtr != NULL) {
        hPtr = Tcl_FindHashEntry(&ioPtr->objectDelegatedFunctions, (char *)
                methodNamePtr);
    } else {
        hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions, (char *)
                methodNamePtr);
    }

    hPtr = NULL;
    if (ioPtr != NULL) {
	if (componentPtr != NULL) {
            Itcl_InitHierIter(&hier, ioPtr->iclsPtr);
	    while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
	        hPtr = Tcl_FindHashEntry(&iclsPtr->components,
	                (char *)componentPtr);
                if (hPtr != NULL) {
	            break;
	        }
	    }
	    Itcl_DeleteHierIter(&hier);
        }
    } else {
	if (componentPtr != NULL) {
	    iclsPtr2 = iclsPtr;
            Itcl_InitHierIter(&hier, iclsPtr2);
	    while ((iclsPtr2 = Itcl_AdvanceHierIter(&hier)) != NULL) {
	        hPtr = Tcl_FindHashEntry(&iclsPtr2->components,
	                (char *)componentPtr);
                if (hPtr != NULL) {
	            break;
	        }
	    }
	    Itcl_DeleteHierIter(&hier);
        }
    }
    if (hPtr == NULL) {
	if (componentPtr != NULL) {
            if (ItclCreateComponent(interp, iclsPtr, componentPtr,
	            ITCL_COMMON, &icPtr) != TCL_OK) {
                return TCL_ERROR;
            }
            hPtr = Tcl_FindHashEntry(&iclsPtr->components,
	            (char *)componentPtr);
        }
    }
    if (hPtr != NULL) {
        icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
    }
    if (*methodName != '*') {
	/* FIXME !!! */
        /* check for locally defined method */
	hPtr = NULL;
	if (ioPtr != NULL) {
	} else {
	    /* FIXME !! have to check the hierarchy !! */
	    hPtr = Tcl_FindHashEntry(&iclsPtr->functions,
	            (char *)methodNamePtr);

	}
	if (hPtr != NULL) {
	    Tcl_AppendResult(interp, "method \"", methodName,
	            "\" has been defined locally", NULL);
	    result = TCL_ERROR;
	    goto errorOut;
	}
    }
    result = ItclCreateDelegatedFunction(interp, iclsPtr, methodNamePtr, icPtr,
            targetPtr, usingPtr, exceptionsPtr, idmPtrPtr);
    (*idmPtrPtr)->flags |= ITCL_METHOD;
errorOut:
    Tcl_DecrRefCount(methodNamePtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassDelegateMethodCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "delegate method" command is invoked to define a
 *  Handles the following syntax:
 *
 *      delegate method
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassDelegateMethodCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclDelegatedFunction *idmPtr;
    int isNew;
    int result;

    ItclShowArgs(1, "Itcl_ClassDelegateMethodCmd", objc, objv);
    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::delegatemethod called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::widget/::itcl::widgetadaptor/::itcl::type",
		"/::itcl::extendedclass.",
		" Only these can delegate methods", NULL);
	return TCL_ERROR;
    }
    result = Itcl_HandleDelegateMethodCmd(interp, NULL, iclsPtr, &idmPtr, objc,
            objv);
    if (result != TCL_OK) {
        return result;
    }
    idmPtr->flags |= ITCL_METHOD;
    hPtr = Tcl_CreateHashEntry(&iclsPtr->delegatedFunctions,
            (char *)idmPtr->namePtr, &isNew);
    Tcl_SetHashValue(hPtr, idmPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_HandleDelegateOptionCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "delegate option" command is invoked to define a delegated option
 *  or if ::itcl::adddelegatedoption is called with an itcl object
 *  Handles the following syntax:
 *
 *      delegate option ...
 *
 * ------------------------------------------------------------------------
 */
int
Itcl_HandleDelegateOptionCmd(
    Tcl_Interp *interp,      /* current interpreter */
    ItclObject *ioPtr,       /* != NULL for ::itcl::adddelgatedoption
                                otherwise NULL */
    ItclClass *iclsPtr,      /* != NULL for delegate option otherwise NULL */
    ItclDelegatedOption **idoPtrPtr,
                             /* where to return idoPtr */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */

{
    Tcl_Obj *allOptionNamePtr;
    Tcl_Obj *optionNamePtr;
    Tcl_Obj *componentPtr;
    Tcl_Obj *targetPtr;
    Tcl_Obj *exceptionsPtr;
    Tcl_Obj *resourceNamePtr;
    Tcl_Obj *classNamePtr;
    Tcl_HashEntry *hPtr;
    ItclComponent *icPtr;
    ItclClass *iclsPtr2;
    ItclDelegatedOption *idoPtr;
    ItclHierIter hier;
    const char *usageStr;
    const char *option;
    const char *component;
    const char *token;
    const char **argv;
    int foundOpt;
    int argc;
    int isStarOption;
    int isNew;
    int i;
    const char *cp;

    ItclShowArgs(1, "Itcl_HandleDelegatedOptionCmd", objc, objv);
    usageStr = "<optionDef> to <targetDef> ?as <script>? ?except <script>?";
    if (objc < 4) {
	Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
        return TCL_ERROR;
    }
    componentPtr = NULL;
    icPtr = NULL;
    isStarOption = 0;
    token = Tcl_GetString(objv[1]);
    if (Tcl_SplitList(interp, (const char *)token, &argc, &argv) != TCL_OK) {
        return TCL_ERROR;
    }
    option = argv[0];
    if (strcmp(option, "*") == 0) {
        isStarOption = 1;
    }
    if ((argc < 1) || (isStarOption && (argc > 1))) {
        Tcl_AppendResult(interp, "<optionDef> must be either \"*\" or ",
	       "\"<optionName> <resourceName> <className>\"", NULL);
	ckfree((char *)argv);
	return TCL_ERROR;
    }
    if (isStarOption && (argc > 3)) {
        Tcl_AppendResult(interp, "<optionDef> syntax should be: ",
	       "\"<optionName> <resourceName> <className>\"", NULL);
	ckfree((char *)argv);
	return TCL_ERROR;
    }
    if ((*option != '-') && !isStarOption) {
	Tcl_AppendResult(interp, "bad delegated option name \"", option,
	        "\", options must start with a \"-\"", NULL);
	ckfree((char *)argv);
        return TCL_ERROR;
    }
    /*
     *  Make sure that the variable name does not contain anything
     *  goofy like a "::" scope qualifier.
     */
    if (strstr(option, "::")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option name \"", option,
	        "\", option names must not contain \"::\"", NULL);
	ckfree((char *)argv);
        return TCL_ERROR;
    }
    if (strstr(option, " ")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                "bad option name \"", option,
	        "\", option names must not contain \" \"", NULL);
	ckfree((char *)argv);
        return TCL_ERROR;
    }
    cp = option;
    while (*cp) {
        if (isupper(UCHAR(*cp))) {
	    Tcl_AppendResult(interp, "bad option name \"", option, "\" ",
	            ", options must not contain uppercase characters", NULL);
	    ckfree((char *)argv);
            return TCL_ERROR;
	}
	cp++;
    }
    optionNamePtr = Tcl_NewStringObj(option, -1);
    Tcl_IncrRefCount(optionNamePtr);
    resourceNamePtr = NULL;
    classNamePtr = NULL;
    if (argc > 1) {
       resourceNamePtr = Tcl_NewStringObj(argv[1], -1);
       Tcl_IncrRefCount(resourceNamePtr);
    }
    if (argc > 2) {
       classNamePtr = Tcl_NewStringObj(argv[2], -1);
    }
    component = NULL;
    targetPtr = NULL;
    exceptionsPtr = NULL;
    for(i=2;i<objc;i++) {
        token = Tcl_GetString(objv[i]);
	if (i+1 == objc) {
	    Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
	    goto errorOut1;
	}
	foundOpt = 0;
	if (strcmp(token, "to") == 0) {
	    i++;
	    component = Tcl_GetString(objv[i]);
	    componentPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "as") == 0) {
	    i++;
	    targetPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "except") == 0) {
	    i++;
	    exceptionsPtr = objv[i];
	    foundOpt++;
        }
        if (!foundOpt) {
	    Tcl_AppendResult(interp, "bad option \"", token, "\" should be ",
	            usageStr, NULL);
	    goto errorOut1;
	}
    }
    if (component == NULL) {
	Tcl_AppendResult(interp, "missing to should be: ", usageStr, NULL);
	goto errorOut1;
    }
    if ((*option == '*') && (targetPtr != NULL)) {
	Tcl_AppendResult(interp,
	        "cannot specify \"as\" with \"delegate option *\"", NULL);
	goto errorOut1;
    }
    /* check for already delegated */
    allOptionNamePtr = Tcl_NewStringObj("*", -1);
    Tcl_IncrRefCount(allOptionNamePtr);
    if (ioPtr != NULL) {
        hPtr = Tcl_FindHashEntry(&ioPtr->objectDelegatedOptions, (char *)
                allOptionNamePtr);
    } else {
        hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedOptions, (char *)
                allOptionNamePtr);
    }
    Tcl_DecrRefCount(allOptionNamePtr);
    if (hPtr != NULL) {
        Tcl_AppendResult(interp, "option \"", option,
	        "\" is already delegated", NULL);
	goto errorOut1;
    }

    if (ioPtr != NULL) {
        Itcl_InitHierIter(&hier, ioPtr->iclsPtr);
	while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
	    hPtr = Tcl_FindHashEntry(&iclsPtr->components,
	            (char *)componentPtr);
            if (hPtr != NULL) {
	        break;
	    }
	}
	Itcl_DeleteHierIter(&hier);
    } else {
        Itcl_InitHierIter(&hier, iclsPtr);
	while ((iclsPtr2 = Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FindHashEntry(&iclsPtr2->components,
	            (char *)componentPtr);
            if (hPtr != NULL) {
	        break;
	    }
	}
	Itcl_DeleteHierIter(&hier);
    }
    if (hPtr == NULL) {
	if (componentPtr != NULL) {
            if (ItclCreateComponent(interp, iclsPtr, componentPtr,
	            ITCL_COMMON, &icPtr) != TCL_OK) {
	        goto errorOut1;
            }
            hPtr = Tcl_FindHashEntry(&iclsPtr->components,
	            (char *)componentPtr);
        }
    }
    if (hPtr != NULL) {
        icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
    }
    if (*option != '*') {
	/* FIXME !!! */
        /* check for valid option name */
	if (ioPtr != NULL) {
	    hPtr = Tcl_FindHashEntry(&ioPtr->objectOptions,
	            (char *)optionNamePtr);
	} else {
            Itcl_InitHierIter(&hier, iclsPtr);
	    while ((iclsPtr2 = Itcl_AdvanceHierIter(&hier)) != NULL) {
	        hPtr = Tcl_FindHashEntry(&iclsPtr2->options,
		        (char *)optionNamePtr);
                if (hPtr != NULL) {
	            break;
	        }
	    }
	}
	if (hPtr != NULL) {
	    Tcl_AppendResult(interp, "option \"", option,
	            "\" has been defined locally", NULL);
	    goto errorOut1;
	    return TCL_ERROR;
	}
    }
    idoPtr = (ItclDelegatedOption *)Itcl_Alloc(sizeof(ItclDelegatedOption));
    Tcl_InitObjHashTable(&idoPtr->exceptions);
    if (*option != '*') {
        if (targetPtr == NULL) {
	    targetPtr = optionNamePtr;
	}
        if (resourceNamePtr == NULL) {
	    resourceNamePtr = Tcl_NewStringObj(option+1, -1);
	    Tcl_IncrRefCount(resourceNamePtr);
	}
        if (classNamePtr == NULL) {
	    classNamePtr = ItclCapitalize(Tcl_GetString(resourceNamePtr));
	}
        idoPtr->namePtr = optionNamePtr;
        idoPtr->resourceNamePtr = resourceNamePtr;
        idoPtr->classNamePtr = Tcl_NewStringObj(
	        Tcl_GetString(classNamePtr), -1);
	Tcl_IncrRefCount(idoPtr->classNamePtr);
	Tcl_DecrRefCount(classNamePtr);

    } else {
        idoPtr->namePtr = optionNamePtr;
    }
    Itcl_PreserveData(idoPtr);
    Itcl_EventuallyFree(idoPtr, (Tcl_FreeProc *) ItclDeleteDelegatedOption);
    idoPtr->icPtr = icPtr;
    idoPtr->asPtr = targetPtr;
    if (idoPtr->asPtr != NULL) {
        Tcl_IncrRefCount(idoPtr->asPtr);
    }
    if (exceptionsPtr != NULL) {
	ckfree((char *)argv);
	argv = NULL;
        if (Tcl_SplitList(interp, Tcl_GetString(exceptionsPtr), &argc, &argv)
	        != TCL_OK) {
	    goto errorOut2;
	}
        for(i=0;i<argc;i++) {
	    Tcl_Obj *objPtr;
	    objPtr = Tcl_NewStringObj(argv[i], -1);
	    hPtr = Tcl_CreateHashEntry(&idoPtr->exceptions, (char *)objPtr,
	            &isNew);
	}
    }
    if (idoPtrPtr != NULL) {
        *idoPtrPtr = idoPtr;
    }
    ckfree((char *)argv);
    ItclAddDelegatedOptionDictInfo(interp, iclsPtr, idoPtr);
    return TCL_OK;
errorOut2:
    Itcl_ReleaseData(idoPtr);
errorOut1:
    Tcl_DecrRefCount(optionNamePtr);
    if (resourceNamePtr != NULL) {
        Tcl_DecrRefCount(resourceNamePtr);
    }
    if (classNamePtr != NULL) {
        Tcl_DecrRefCount(classNamePtr);
    }
    if (argv) {
	ckfree((char *)argv);
    }
    return TCL_ERROR;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassDelegateOptionCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "delegate option" command is invoked to define a
 *  Handles the following syntax:
 *
 *      delegate option
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassDelegateOptionCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclDelegatedOption *idoPtr;
    const char *usageStr;
    int isNew;
    int result;

    ItclShowArgs(1, "Itcl_ClassDelegateOptionCmd", objc, objv);
    usageStr = "<optionDef> to <targetDef> ?as <script>? ?except <script>?";
    if (objc < 4) {
	Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
        return TCL_ERROR;
    }
    infoPtr = (ItclObjectInfo *)clientData;
    iclsPtr = (ItclClass *)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::delegateoption called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::widget/::itcl::widgetadaptor/::itcl::type",
		"/::itcl::extendedclass.",
		" Only these can delegate options", NULL);
	return TCL_ERROR;
    }
    result = Itcl_HandleDelegateOptionCmd(interp, NULL, iclsPtr, &idoPtr,
             objc, objv);
    if (result != TCL_OK) {
        return result;
    }
    hPtr = Tcl_CreateHashEntry(&iclsPtr->delegatedOptions,
            (char *)idoPtr->namePtr, &isNew);
    Tcl_SetHashValue(hPtr, idoPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassDelegateTypeMethodCmd()
 *
 *  Invoked by Tcl during the parsing of a class definition whenever
 *  the "delegate typemethod" command is invoked to define a
 *  Handles the following syntax:
 *
 *      delegate typemethod
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassDelegateTypeMethodCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *typeMethodNamePtr;
    Tcl_Obj *componentPtr;
    Tcl_Obj *targetPtr;
    Tcl_Obj *usingPtr;
    Tcl_Obj *exceptionsPtr;
    Tcl_HashEntry *hPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclComponent *icPtr;
    ItclDelegatedFunction *idmPtr;
    const char *usageStr;
    const char *typeMethodName;
    const char *component;
    const char *token;
    const char **argv;
    int foundOpt;
    int argc;
    int isNew;
    int i;

    ItclShowArgs(1, "Itcl_ClassDelegateTypeMethodCmd", objc, objv);
    usageStr = "delegate typemethod <typeMethodName> to <componentName> ?as <targetName>?\n\
delegate typemethod <typeMethodName> ?to <componentName>? using <pattern>\n\
delegate typemethod * ?to <componentName>? ?using <pattern>? ?except <typemethods>?";
    componentPtr = NULL;
    icPtr = NULL;
    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::delegatetypemethod called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::widget/::itcl::widgetadaptor/::itcl::type.",
		" Only these can delegate typemethods", NULL);
	return TCL_ERROR;
    }

    if (objc < 4) {
	Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
        return TCL_ERROR;
    }
    typeMethodName = Tcl_GetString(objv[1]);
    /* check if typeMethodName has been delegated */
    component = NULL;
    targetPtr = NULL;
    usingPtr = NULL;
    exceptionsPtr = NULL;
    for(i=2;i<objc;i++) {
        token = Tcl_GetString(objv[i]);
	if (i+1 == objc) {
	    Tcl_AppendResult(interp, "wrong # args should be ", usageStr, NULL);
	    return TCL_ERROR;
	}
	foundOpt = 0;
	if (strcmp(token, "to") == 0) {
	    i++;
	    component = Tcl_GetString(objv[i]);
	    componentPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "as") == 0) {
	    i++;
	    targetPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "except") == 0) {
	    i++;
	    exceptionsPtr = objv[i];
	    foundOpt++;
        }
	if (strcmp(token, "using") == 0) {
	    i++;
	    usingPtr = objv[i];
	    foundOpt++;
        }
        if (!foundOpt) {
	    Tcl_AppendResult(interp, "bad option \"", token, "\" should be ",
	            usageStr, NULL);
	    return TCL_ERROR;
	}
    }
    if ((component == NULL) && (usingPtr == NULL)) {
	Tcl_AppendResult(interp, "missing to should be: ", usageStr, NULL);
	return TCL_ERROR;
    }
    if ((*typeMethodName == '*') && (targetPtr != NULL)) {
	Tcl_AppendResult(interp,
	        "cannot specify \"as\" with \"delegate typemethod *\"", NULL);
	return TCL_ERROR;
    }
    if (componentPtr != NULL) {
	hPtr = Tcl_FindHashEntry(&iclsPtr->components, (char *)componentPtr);
	if (hPtr == NULL) {
            if (ItclCreateComponent(interp, iclsPtr, componentPtr,
	            ITCL_COMMON, &icPtr) != TCL_OK) {
                return TCL_ERROR;
            }
        } else {
	    icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
	}
    } else {
        icPtr = NULL;
    }
    idmPtr = (ItclDelegatedFunction *)ckalloc(sizeof(ItclDelegatedFunction));
    memset(idmPtr, 0, sizeof(ItclDelegatedFunction));
    Tcl_InitObjHashTable(&idmPtr->exceptions);
    typeMethodNamePtr = Tcl_NewStringObj(typeMethodName, -1);
    if (*typeMethodName != '*') {
	/* FIXME !!! */
        /* check for locally defined typemethod */
	hPtr = Tcl_FindHashEntry(&iclsPtr->functions,
	        (char *)typeMethodNamePtr);
	if (hPtr != NULL) {
	    Tcl_AppendResult(interp, "Error in \"delegate typemethod ",
	            typeMethodName, "...\", \"", typeMethodName,
	            "\" has been defined locally.", NULL);
            Tcl_DeleteHashTable(&idmPtr->exceptions);
	    ckfree((char *)idmPtr);
	    Tcl_DecrRefCount(typeMethodNamePtr);
	    return TCL_ERROR;
	}
        idmPtr->namePtr = Tcl_NewStringObj(
	        Tcl_GetString(typeMethodNamePtr), -1);
        Tcl_IncrRefCount(idmPtr->namePtr);
    } else {
	Tcl_DecrRefCount(typeMethodNamePtr);
        typeMethodNamePtr = Tcl_NewStringObj("*", -1);
	Tcl_IncrRefCount(typeMethodNamePtr);
        idmPtr->namePtr = typeMethodNamePtr;
	Tcl_IncrRefCount(typeMethodNamePtr);
        if (exceptionsPtr != NULL) {
            if (Tcl_SplitList(interp, Tcl_GetString(exceptionsPtr),
	            &argc, &argv) != TCL_OK) {
	        return TCL_ERROR;
	    }
            for(i = 0; i < argc; i++) {
	        Tcl_Obj *objPtr;
	        objPtr = Tcl_NewStringObj(argv[i], -1);
	        hPtr = Tcl_CreateHashEntry(&idmPtr->exceptions, (char *)objPtr,
	                &isNew);
	    }
	    ckfree((char *) argv);
        }
    }
    idmPtr->icPtr = icPtr;
    idmPtr->asPtr = targetPtr;
    if (idmPtr->asPtr != NULL) {
        Tcl_IncrRefCount(idmPtr->asPtr);
    }
    idmPtr->usingPtr = usingPtr;
    if (idmPtr->usingPtr != NULL) {
        Tcl_IncrRefCount(idmPtr->usingPtr);
    }
    idmPtr->flags = ITCL_COMMON|ITCL_TYPE_METHOD;
    hPtr = Tcl_CreateHashEntry(&iclsPtr->delegatedFunctions,
            (char *)idmPtr->namePtr, &isNew);
    if (!isNew) {
        ItclDeleteDelegatedFunction((ItclDelegatedFunction *)
	        Tcl_GetHashValue(hPtr));
    }
    Tcl_SetHashValue(hPtr, idmPtr);
    Tcl_DecrRefCount(typeMethodNamePtr);
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassForwardCmd()
 *
 *  Used similar to interp alias to forward the call of a method
 *  to another method within the class
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_ClassForwardCmd(
    ClientData clientData,   /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *prefixObj;
    Tcl_Method mPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;

    ItclShowArgs(1, "Itcl_ClassForwardCmd", objc, objv);
    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::forward called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::widget/::itcl::widgetadaptor/",
		"::itcl::type/::itcl::extendedclass.",
		" Only these can forward", NULL);
	return TCL_ERROR;
    }
    if (objc < 3) {
        Tcl_WrongNumArgs(interp, 1, objv, "<forwardName> <targetName> ?<arg> ...?");
        return TCL_ERROR;
    }
    prefixObj = Tcl_NewListObj(objc-2, objv+2);
    mPtr = Itcl_NewForwardClassMethod(interp, iclsPtr->clsPtr, 1,
            objv[1], prefixObj);
    if (mPtr == NULL) {
        return TCL_ERROR;
    }
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassMethodVariableCmd()
 *
 *  Used to similar to iterp alias to forward the call of a method
 *  to another method within the class
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_ClassMethodVariableCmd(
    ClientData clientData,   /* unused */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *namePtr;
    Tcl_Obj *defaultPtr;
    Tcl_Obj *callbackPtr;
    ItclObjectInfo *infoPtr;
    ItclClass *iclsPtr;
    ItclVariable *ivPtr;
    ItclMemberFunc *imPtr;
    ItclMethodVariable *imvPtr;
    const char *token;
    const char *usageStr;
    int i;
    int foundOpt;
    int result;
    Tcl_Obj *objPtr;

    ItclShowArgs(1, "Itcl_ClassMethodVariableCmd", objc, objv);
    infoPtr = (ItclObjectInfo*)clientData;
    iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::methodvariable called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "\"", Tcl_GetString(iclsPtr->namePtr),
	        " is no ::itcl::widget/::itcl::widgetadaptor/",
		"::itcl::type/::itcl::extendedclass.",
		" Only these can have methodvariables", NULL);
	return TCL_ERROR;
    }
    usageStr = "<name> ?-default value? ?-callback script?";
    if ((objc < 2) || (objc > 6)) {
        Tcl_WrongNumArgs(interp, 1, objv, usageStr);
        return TCL_ERROR;
    }

    /*
     *  Make sure that the variable name does not contain anything
     *  goofy like a "::" scope qualifier.
     */
    namePtr = objv[1];
    if (strstr(Tcl_GetString(namePtr), "::")) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "bad variable name \"", Tcl_GetString(namePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    defaultPtr = NULL;
    callbackPtr = NULL;
    for (i=2;i<objc;i++) {
	foundOpt = 0;
        token = Tcl_GetString(objv[i]);
	if (strcmp(token, "-default") == 0) {
	    if (i+1 > objc) {
                Tcl_WrongNumArgs(interp, 1, objv, usageStr);
                return TCL_ERROR;
	    }
	    defaultPtr = objv[i+1];
	    i++;
	    foundOpt++;
	}
	if (strcmp(token, "-callback") == 0) {
	    if (i+1 > objc) {
                Tcl_WrongNumArgs(interp, 1, objv, usageStr);
                return TCL_ERROR;
	    }
	    callbackPtr = objv[i+1];
	    i++;
	    foundOpt++;
	}
	if (!foundOpt) {
            Tcl_WrongNumArgs(interp, 1, objv, usageStr);
            return TCL_ERROR;
        }
    }

    if (Itcl_CreateVariable(interp, iclsPtr, namePtr,
            Tcl_GetString(defaultPtr), NULL, &ivPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    iclsPtr->numVariables++;
    result = ItclCreateMethodVariable(interp, ivPtr, defaultPtr,
            callbackPtr, &imvPtr);
    if (result != TCL_OK) {
        return result;
    }
    objPtr = Tcl_NewStringObj("@itcl-builtin-setget ", -1);
    Tcl_AppendToObj(objPtr, Tcl_GetString(namePtr), -1);
    Tcl_AppendToObj(objPtr, " ", 1);
    result = ItclCreateMethod(interp, iclsPtr, namePtr, "args",
            Tcl_GetString(objPtr), &imPtr);
    if (result != TCL_OK) {
        return result;
    }
    /* install a write trace if callbackPtr != NULL */
    /* FIXME to be done */
    ItclAddClassVariableDictInfo(interp, iclsPtr, ivPtr);
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_ClassTypeConstructorCmd()
 *
 *  Invoked by Tcl during the parsing of a type class definition whenever
 *  the "typeconstructor" command is invoked to define the typeconstructor
 *  for an object.  Handles the following syntax:
 *
 *      typeconstructor <body>
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_ClassTypeConstructorCmd(
    ClientData clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr = (ItclClass*)Itcl_PeekStack(&infoPtr->clsStack);
    Tcl_Obj *namePtr;

    ItclShowArgs(1, "Itcl_ClassTypeConstructorCmd", objc, objv);

    if (iclsPtr == NULL) {
        Tcl_AppendResult(interp, "Error: ::itcl::parser::typeconstructor called from",
	        " not within a class", NULL);
        return TCL_ERROR;
    }
    if (iclsPtr->flags & ITCL_CLASS) {
        Tcl_AppendResult(interp, "a \"class\" cannot have a typeconstructor",
	        NULL);
	return TCL_ERROR;
    }

    if (objc != 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "body");
        return TCL_ERROR;
    }

    namePtr = objv[0];
    if (iclsPtr->typeConstructorPtr != NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "\"", Tcl_GetString(namePtr), "\" already defined in class \"",
            Tcl_GetString(iclsPtr->fullNamePtr), "\"",
            NULL);
        return TCL_ERROR;
    }

    iclsPtr->typeConstructorPtr = Tcl_NewStringObj(Tcl_GetString(objv[1]), -1);
    Tcl_IncrRefCount(iclsPtr->typeConstructorPtr);
    return TCL_OK;
}
