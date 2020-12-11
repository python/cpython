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
 *  These procedures handle built-in class methods, including the
 *  "isa" method (to query hierarchy info) and the "info" method
 *  (to query class/object data).
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

static char initHullCmdsScript[] =
"namespace eval ::itcl {\n"
"    proc _find_hull_init {} {\n"
"        global env tcl_library\n"
"        variable library\n"
"        variable patchLevel\n"
"        rename _find_hull_init {}\n"
"        if {[info exists library]} {\n"
"            lappend dirs $library\n"
"        } else {\n"
"            set dirs {}\n"
"            if {[info exists env(ITCL_LIBRARY)]} {\n"
"                lappend dirs $env(ITCL_LIBRARY)\n"
"            }\n"
"            lappend dirs [file join [file dirname $tcl_library] itcl$patchLevel]\n"
"            set bindir [file dirname [info nameofexecutable]]\n"
"	    lappend dirs [file join . library]\n"
"            lappend dirs [file join $bindir .. lib itcl$patchLevel]\n"
"            lappend dirs [file join $bindir .. library]\n"
"            lappend dirs [file join $bindir .. .. library]\n"
"            lappend dirs [file join $bindir .. .. itcl library]\n"
"            lappend dirs [file join $bindir .. .. .. itcl library]\n"
"            lappend dirs [file join $bindir .. .. itcl-ng itcl library]\n"
"            # On MacOSX, check the directories in the tcl_pkgPath\n"
"            if {[string equal $::tcl_platform(platform) \"unix\"] && "
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
"            set itclfile [file join $i itclHullCmds.tcl]\n"
"            if {![catch {uplevel #0 [list source $itclfile]} msg]} {\n"
"                return\n"
"            }\n"
"puts stderr \"MSG!$msg!\"\n"
"        }\n"
"        set msg \"Can't find a usable itclHullCmds.tcl in the following directories:\n\"\n"
"        append msg \"    $dirs\n\"\n"
"        append msg \"This probably means that Itcl/Tcl weren't installed properly.\n\"\n"
"        append msg \"If you know where the Itcl library directory was installed,\n\"\n"
"        append msg \"you can set the environment variable ITCL_LIBRARY to point\n\"\n"
"        append msg \"to the library directory.\n\"\n"
"        error $msg\n"
"    }\n"
"    _find_hull_init\n"
"}";

static Tcl_ObjCmdProc Itcl_BiDestroyCmd;
static Tcl_ObjCmdProc ItclExtendedConfigure;
static Tcl_ObjCmdProc ItclExtendedCget;
static Tcl_ObjCmdProc ItclExtendedSetGet;
static Tcl_ObjCmdProc Itcl_BiCreateHullCmd;
static Tcl_ObjCmdProc Itcl_BiSetupComponentCmd;
static Tcl_ObjCmdProc Itcl_BiKeepComponentOptionCmd;
static Tcl_ObjCmdProc Itcl_BiIgnoreComponentOptionCmd;
static Tcl_ObjCmdProc Itcl_BiInitOptionsCmd;

/*
 *  FORWARD DECLARATIONS
 */
static Tcl_Obj* ItclReportPublicOpt(Tcl_Interp *interp,
    ItclVariable *ivPtr, ItclObject *contextIoPtr);

static Tcl_ObjCmdProc ItclBiClassUnknownCmd;
/*
 *  Standard list of built-in methods for all objects.
 */
typedef struct BiMethod {
    const char* name;        /* method name */
    const char* usage;       /* string describing usage */
    const char* registration;/* registration name for C proc */
    Tcl_ObjCmdProc *proc;    /* implementation C proc */
    int flags;               /* flag for which type of class to be used */
} BiMethod;

static const BiMethod BiMethodList[] = {
    { "callinstance",
        "<instancename>",
        "@itcl-builtin-callinstance",
        Itcl_BiCallInstanceCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "getinstancevar",
        "<instancename>",
        "@itcl-builtin-getinstancevar",
        Itcl_BiGetInstanceVarCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "cget",
        "-option",
        "@itcl-builtin-cget",
        Itcl_BiCgetCmd,
	ITCL_CLASS|ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "configure",
        "?-option? ?value -option value...?",
        "@itcl-builtin-configure",
        Itcl_BiConfigureCmd,
	ITCL_CLASS|ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    {"createhull",
        "widgetType widgetPath ?-class className? ?optionName value ...?",
        "@itcl-builtin-createhull",
        Itcl_BiCreateHullCmd,
	ITCL_ECLASS
    },
    { "destroy",
        "",
        "@itcl-builtin-destroy",
        Itcl_BiDestroyCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "installcomponent",
        "<componentName> using <classname> <winpath> ?-option value...?",
        "@itcl-builtin-installcomponent",
        Itcl_BiInstallComponentCmd,
	ITCL_WIDGET
    },
    { "itcl_hull",
        "",
        "@itcl-builtin-itcl_hull",
        Itcl_BiItclHullCmd,
	ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "isa",
        "className",
        "@itcl-builtin-isa",
        Itcl_BiIsaCmd,
	ITCL_CLASS|ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET
    },
    {"itcl_initoptions",
        "?optionName value ...?",
        "@itcl-builtin-initoptions",
        Itcl_BiInitOptionsCmd,
	ITCL_ECLASS
    },
    { "mymethod",
        "",
        "@itcl-builtin-mymethod",
        Itcl_BiMyMethodCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "myvar",
        "",
        "@itcl-builtin-myvar",
        Itcl_BiMyVarCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "myproc",
        "",
        "@itcl-builtin-myproc",
        Itcl_BiMyProcCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "mytypemethod",
        "",
        "@itcl-builtin-mytypemethod",
        Itcl_BiMyTypeMethodCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "mytypevar",
        "",
        "@itcl-builtin-mytypevar",
        Itcl_BiMyTypeVarCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    { "setget",
        "varName ?value?",
        "@itcl-builtin-setget",
        ItclExtendedSetGet,
	ITCL_ECLASS
    },
    { "unknown",
        "",
        "@itcl-builtin-classunknown",
        ItclBiClassUnknownCmd,
	ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR
    },
    {"keepcomponentoption",
        "componentName optionName ?optionName ...?",
        "@itcl-builtin-keepcomponentoption",
        Itcl_BiKeepComponentOptionCmd,
	ITCL_ECLASS
    },
    {"ignorecomponentoption",
        "componentName optionName ?optionName ...?",
        "@itcl-builtin-ignorecomponentoption",
        Itcl_BiIgnoreComponentOptionCmd,
	ITCL_ECLASS
    },
    /* the next 3 are defined in library/itclHullCmds.tcl */
    {"addoptioncomponent",
        "componentName optionName ?optionName ...?",
        "@itcl-builtin-addoptioncomponent",
        NULL,
	ITCL_ECLASS
    },
    {"ignoreoptioncomponent",
        "componentName optionName ?optionName ...?",
        "@itcl-builtin-ignoreoptioncomponent",
        NULL,
	ITCL_ECLASS
    },
    {"renameoptioncomponent",
        "componentName optionName ?optionName ...?",
        "@itcl-builtin-renameoptioncomponent",
        NULL,
	ITCL_ECLASS
    },
    {"setupcomponent",
        "componentName using widgetType widgetPath ?optionName value ...?",
        "@itcl-builtin-setupcomponent",
        Itcl_BiSetupComponentCmd,
	ITCL_ECLASS
    },
};
static int BiMethodListLen = sizeof(BiMethodList)/sizeof(BiMethod);


/*
 * ------------------------------------------------------------------------
 *  ItclRestoreInfoVars()
 *
 *  Delete callback to restore original "info" ensemble (revert inject of Itcl)
 *
 * ------------------------------------------------------------------------
 */

void
ItclRestoreInfoVars(
    ClientData clientData)
{
    ItclObjectInfo *infoPtr = (ItclObjectInfo *)clientData;
    Tcl_Interp *interp = infoPtr->interp;
    Tcl_Command cmd;
    Tcl_Obj *mapDict;

    cmd = Tcl_FindCommand(interp, "info", NULL, TCL_GLOBAL_ONLY);
    if (cmd == NULL || !Tcl_IsEnsemble(cmd)) {
	goto done;
    }
    Tcl_GetEnsembleMappingDict(NULL, cmd, &mapDict);
    if (mapDict == NULL) {
	goto done;
    }
    if (infoPtr->infoVarsPtr == NULL || infoPtr->infoVars4Ptr == NULL) {
	/* Safety */
	goto done;
    }
    Tcl_DictObjPut(NULL, mapDict, infoPtr->infoVars4Ptr, infoPtr->infoVarsPtr);
    Tcl_SetEnsembleMappingDict(interp, cmd, mapDict);

done:
    if (infoPtr->infoVarsPtr) {
	Tcl_DecrRefCount(infoPtr->infoVarsPtr);
	infoPtr->infoVarsPtr = NULL;
    }
    if (infoPtr->infoVars4Ptr) {
	Tcl_DecrRefCount(infoPtr->infoVars4Ptr);
	infoPtr->infoVars4Ptr = NULL;
    }
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInit()
 *
 *  Creates a namespace full of built-in methods/procs for [incr Tcl]
 *  classes.  This includes things like the "isa" method and "info"
 *  for querying class info.  Usually invoked by Itcl_Init() when
 *  [incr Tcl] is first installed into an interpreter.
 *
 *  Returns TCL_OK/TCL_ERROR to indicate success/failure.
 * ------------------------------------------------------------------------
 */

int
Itcl_BiInit(
    Tcl_Interp *interp,      /* current interpreter */
    ItclObjectInfo *infoPtr)
{
    Tcl_Namespace *itclBiNs;
    Tcl_DString buffer;
    Tcl_Obj *mapDict;
    Tcl_Command infoCmd;
    int result;
    int i;

    /*
     *  "::itcl::builtin" commands.
     *  These commands are imported into each class
     *  just before the class definition is parsed.
     */
    Tcl_DStringInit(&buffer);
    for (i=0; i < BiMethodListLen; i++) {
	Tcl_DStringSetLength(&buffer, 0);
	Tcl_DStringAppend(&buffer, "::itcl::builtin::", -1);
	Tcl_DStringAppend(&buffer, BiMethodList[i].name, -1);
        Tcl_CreateObjCommand(interp, Tcl_DStringValue(&buffer),
	        BiMethodList[i].proc, infoPtr, NULL);
    }
    Tcl_DStringFree(&buffer);

    Tcl_CreateObjCommand(interp, "::itcl::builtin::chain", Itcl_BiChainCmd,
            NULL, NULL);

    Tcl_CreateObjCommand(interp, "::itcl::builtin::classunknown",
            ItclBiClassUnknownCmd, infoPtr, NULL);

    ItclInfoInit(interp, infoPtr);
    /*
     *  Export all commands in the built-in namespace so we can
     *  import them later on.
     */
    itclBiNs = Tcl_FindNamespace(interp, "::itcl::builtin",
        NULL, TCL_LEAVE_ERR_MSG);

    if ((itclBiNs == NULL) ||
        Tcl_Export(interp, itclBiNs, "[a-z]*", /* resetListFirst */ 1) != TCL_OK) {
        return TCL_ERROR;
    }
    /*
     * Install into the master [info] ensemble.
     */

    infoCmd = Tcl_FindCommand(interp, "info", NULL, TCL_GLOBAL_ONLY);
    if (infoCmd != NULL && Tcl_IsEnsemble(infoCmd)) {
        Tcl_GetEnsembleMappingDict(NULL, infoCmd, &mapDict);
        if (mapDict != NULL) {
            infoPtr->infoVars4Ptr = Tcl_NewStringObj("vars", -1);
	    Tcl_IncrRefCount(infoPtr->infoVars4Ptr);
            result = Tcl_DictObjGet(NULL, mapDict, infoPtr->infoVars4Ptr,
	            &infoPtr->infoVarsPtr);
	    if (result == TCL_OK && infoPtr->infoVarsPtr) {
		Tcl_IncrRefCount(infoPtr->infoVarsPtr);
        	Tcl_DictObjPut(NULL, mapDict, infoPtr->infoVars4Ptr,
	        	Tcl_NewStringObj("::itcl::builtin::Info::vars", -1));
        	Tcl_SetEnsembleMappingDict(interp, infoCmd, mapDict);
        	/*
        	 * Note that ItclRestoreInfoVars is called in callback
        	 * if built-in Itcl command info::vars or the ensemble get
        	 * deleted (see ItclInfoInit registering that). */
	    } else {
		Tcl_DecrRefCount(infoPtr->infoVars4Ptr);
		infoPtr->infoVars4Ptr = NULL;
	    }
        }
    }

    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_InstallBiMethods()
 *
 *  Invoked when a class is first created, just after the class
 *  definition has been parsed, to add definitions for built-in
 *  methods to the class.  If a method already exists in the class
 *  with the same name as the built-in, then the built-in is skipped.
 *  Otherwise, a method definition for the built-in method is added.
 *
 *  Returns TCL_OK if successful, or TCL_ERROR (along with an error
 *  message in the interpreter) if anything goes wrong.
 * ------------------------------------------------------------------------
 */
int
Itcl_InstallBiMethods(
    Tcl_Interp *interp,      /* current interpreter */
    ItclClass *iclsPtr)      /* class definition to be updated */
{
    int result = TCL_OK;

    int i;
    ItclHierIter hier;
    ItclClass *superPtr;

    /*
     *  Scan through all of the built-in methods and see if
     *  that method already exists in the class.  If not, add
     *  it in.
     *
     *  TRICKY NOTE:  The virtual tables haven't been built yet,
     *    so look for existing methods the hard way--by scanning
     *    through all classes.
     */
    Tcl_Obj *objPtr = Tcl_NewStringObj("", 0);
    for (i=0; i < BiMethodListLen; i++) {
	Tcl_HashEntry *hPtr = NULL;

        Itcl_InitHierIter(&hier, iclsPtr);
	Tcl_SetStringObj(objPtr, BiMethodList[i].name, -1);
        superPtr = Itcl_AdvanceHierIter(&hier);
        while (superPtr) {
            hPtr = Tcl_FindHashEntry(&superPtr->functions, (char *)objPtr);
            if (hPtr) {
                break;
            }
            superPtr = Itcl_AdvanceHierIter(&hier);
        }
        Itcl_DeleteHierIter(&hier);

        if (!hPtr) {
	    if (iclsPtr->flags & BiMethodList[i].flags) {
                result = Itcl_CreateMethod(interp, iclsPtr,
	            Tcl_NewStringObj(BiMethodList[i].name, -1),
                    BiMethodList[i].usage, BiMethodList[i].registration);

                if (result != TCL_OK) {
                    break;
                }
	    }
        }
    }

    /*
     * Every Itcl class gets an info method installed so that each has
     * a proper context for the subcommands to do their context senstive
     * work.
     */

    if (result == TCL_OK
	    && (iclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
	result = Itcl_CreateMethod(interp, iclsPtr,
		Tcl_NewStringObj("info", -1), NULL, "@itcl-builtin-info");
    }

    Tcl_DecrRefCount(objPtr);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiIsaCmd()
 *
 *  Invoked whenever the user issues the "isa" method for an object.
 *  Handles the following syntax:
 *
 *    <objName> isa <className>
 *
 *  Checks to see if the object has the given <className> anywhere
 *  in its heritage.  Returns 1 if so, and 0 otherwise.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiIsaCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *iclsPtr;
    const char *token;

    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (contextIoPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "improper usage: should be \"object isa className\"",
            NULL);
        return TCL_ERROR;
    }
    if (objc != 2) {
        token = Tcl_GetString(objv[0]);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"object ", token, " className\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  Look for the requested class.  If it is not found, then
     *  try to autoload it.  If it absolutely cannot be found,
     *  signal an error.
     */
    token = Tcl_GetString(objv[1]);
    iclsPtr = Itcl_FindClass(interp, token, /* autoload */ 1);
    if (iclsPtr == NULL) {
        return TCL_ERROR;
    }

    if (Itcl_ObjectIsa(contextIoPtr, iclsPtr)) {
        Tcl_SetWideIntObj(Tcl_GetObjResult(interp), 1);
    } else {
        Tcl_SetWideIntObj(Tcl_GetObjResult(interp), 0);
    }
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiConfigureCmd()
 *
 *  Invoked whenever the user issues the "configure" method for an object.
 *  Handles the following syntax:
 *
 *    <objName> configure ?-<option>? ?<value> -<option> <value>...?
 *
 *  Allows access to public variables as if they were configuration
 *  options.  With no arguments, this command returns the current
 *  list of public variable options.  If -<option> is specified,
 *  this returns the information for just one option:
 *
 *    -<optionName> <initVal> <currentVal>
 *
 *  Otherwise, the list of arguments is parsed, and values are
 *  assigned to the various public variable options.  When each
 *  option changes, a big of "config" code associated with the option
 *  is executed, to bring the object up to date.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiConfigureCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_DString buffer;
    Tcl_DString buffer2;
    Tcl_HashSearch place;
    Tcl_HashEntry *hPtr;
    Tcl_Namespace *saveNsPtr;
    Tcl_Obj * const *unparsedObjv;
    ItclClass *iclsPtr;
    ItclVariable *ivPtr;
    ItclVarLookup *vlookup;
    ItclMemberCode *mcode;
    ItclHierIter hier;
    ItclObjectInfo *infoPtr;
    const char *lastval;
    const char *token;
    char *varName;
    int i;
    int unparsedObjc;
    int result;

    ItclShowArgs(1, "Itcl_BiConfigureCmd", objc, objv);
    vlookup = NULL;
    token = NULL;
    hPtr = NULL;
    unparsedObjc = objc;
    unparsedObjv = objv;
    Tcl_DStringInit(&buffer);
    Tcl_DStringInit(&buffer2);

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (contextIoPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "improper usage: should be ",
            "\"object configure ?-option? ?value -option value...?\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  BE CAREFUL:  work in the virtual scope!
     */
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    infoPtr = contextIclsPtr->infoPtr;
    if (!(contextIclsPtr->flags & ITCL_CLASS)) {
	/* first check if it is an option */
	if (objc > 1) {
            hPtr = Tcl_FindHashEntry(&contextIclsPtr->options,
	            (char *) objv[1]);
	}
        result = ItclExtendedConfigure(contextIclsPtr, interp, objc, objv);
        if (result != TCL_CONTINUE) {
            return result;
        }
        if (infoPtr->unparsedObjc > 0) {
            unparsedObjc = infoPtr->unparsedObjc;
            unparsedObjv = infoPtr->unparsedObjv;
	} else {
	    unparsedObjc = objc;
	}
    }
    /*
     *  HANDLE:  configure
     */
    if (unparsedObjc == 1) {
        resultPtr = Tcl_NewListObj(0, NULL);

        Itcl_InitHierIter(&hier, contextIclsPtr);
        while ((iclsPtr=Itcl_AdvanceHierIter(&hier)) != NULL) {
            hPtr = Tcl_FirstHashEntry(&iclsPtr->variables, &place);
            while (hPtr) {
                ivPtr = (ItclVariable*)Tcl_GetHashValue(hPtr);
                if (ivPtr->protection == ITCL_PUBLIC) {
                    objPtr = ItclReportPublicOpt(interp, ivPtr, contextIoPtr);

                    Tcl_ListObjAppendElement(NULL, resultPtr,
                        objPtr);
                }
                hPtr = Tcl_NextHashEntry(&place);
            }
        }
        Itcl_DeleteHierIter(&hier);

        Tcl_SetObjResult(interp, resultPtr);
        return TCL_OK;
    } else {

        /*
         *  HANDLE:  configure -option
         */
        if (unparsedObjc == 2) {
            token = Tcl_GetString(unparsedObjv[1]);
            if (*token != '-') {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "improper usage: should be ",
                    "\"object configure ?-option? ?value -option value...?\"",
                    NULL);
                return TCL_ERROR;
            }

            vlookup = NULL;
            hPtr = ItclResolveVarEntry(contextIclsPtr, token+1);
            if (hPtr) {
                vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);

                if (vlookup->ivPtr->protection != ITCL_PUBLIC) {
                    vlookup = NULL;
                }
            }
            if (!vlookup) {
                Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                    "unknown option \"", token, "\"",
                    NULL);
                return TCL_ERROR;
            }
            resultPtr = ItclReportPublicOpt(interp,
	            vlookup->ivPtr, contextIoPtr);
            Tcl_SetObjResult(interp, resultPtr);
            return TCL_OK;
        }
    }

    /*
     *  HANDLE:  configure -option value -option value...
     *
     *  Be careful to work in the virtual scope.  If this "configure"
     *  method was defined in a base class, the current namespace
     *  (from Itcl_ExecMethod()) will be that base class.  Activate
     *  the derived class namespace here, so that instance variables
     *  are accessed properly.
     */
    result = TCL_OK;

    for (i=1; i < unparsedObjc; i+=2) {
	if (i+1 >= unparsedObjc) {
	    Tcl_AppendResult(interp, "need option value pair", NULL);
	    result = TCL_ERROR;
            goto configureDone;
	}
        vlookup = NULL;
        token = Tcl_GetString(unparsedObjv[i]);
        if (*token == '-') {
            hPtr = ItclResolveVarEntry(contextIclsPtr, token+1);
            if (hPtr == NULL) {
                hPtr = ItclResolveVarEntry(contextIclsPtr, token);
	    }
            if (hPtr) {
                vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
            }
        }

        if (!vlookup || (vlookup->ivPtr->protection != ITCL_PUBLIC)) {
            Tcl_AppendResult(interp, "unknown option \"", token, "\"",
                NULL);
            result = TCL_ERROR;
            goto configureDone;
        }
        if (i == unparsedObjc-1) {
            Tcl_AppendResult(interp, "value for \"", token, "\" missing",
                NULL);
            result = TCL_ERROR;
            goto configureDone;
        }

        ivPtr = vlookup->ivPtr;
        Tcl_DStringSetLength(&buffer2, 0);
	if (!(ivPtr->flags & ITCL_COMMON)) {
            Tcl_DStringAppend(&buffer2,
	            Tcl_GetString(contextIoPtr->varNsNamePtr), -1);
	}
        Tcl_DStringAppend(&buffer2,
	        Tcl_GetString(ivPtr->iclsPtr->fullNamePtr), -1);
        Tcl_DStringAppend(&buffer2, "::", 2);
        Tcl_DStringAppend(&buffer2,
	        Tcl_GetString(ivPtr->namePtr), -1);
	varName = Tcl_DStringValue(&buffer2);
        lastval = Tcl_GetVar2(interp, varName, NULL, 0);
        Tcl_DStringSetLength(&buffer, 0);
        Tcl_DStringAppend(&buffer, (lastval) ? lastval : "", -1);

        token = Tcl_GetString(unparsedObjv[i+1]);
        if (Tcl_SetVar2(interp, varName, NULL, token,
                TCL_LEAVE_ERR_MSG) == NULL) {
    	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
    		    "\n    (error in configuration of public variable \"%s\")",
    		    Tcl_GetString(ivPtr->fullNamePtr)));
            result = TCL_ERROR;
            goto configureDone;
        }

        /*
         *  If this variable has some "config" code, invoke it now.
         *
         *  TRICKY NOTE:  Be careful to evaluate the code one level
         *    up in the call stack, so that it's executed in the
         *    calling context, and not in the context that we've
         *    set up for public variable access.
         */
        mcode = ivPtr->codePtr;
        if (mcode && Itcl_IsMemberCodeImplemented(mcode)) {
	    if (!ivPtr->iclsPtr->infoPtr->useOldResolvers) {
                Itcl_SetCallFrameResolver(interp, contextIoPtr->resolvePtr);
            }
	    saveNsPtr = Tcl_GetCurrentNamespace(interp);
	    Itcl_SetCallFrameNamespace(interp, ivPtr->iclsPtr->nsPtr);
	    result = Tcl_EvalObjEx(interp, mcode->bodyPtr, 0);
	    Itcl_SetCallFrameNamespace(interp, saveNsPtr);
            if (result == TCL_OK) {
                Tcl_ResetResult(interp);
            } else {
        	    Tcl_AppendObjToErrorInfo(interp, Tcl_ObjPrintf(
        		    "\n    (error in configuration of public variable \"%s\")",
        		    Tcl_GetString(ivPtr->fullNamePtr)));
                Tcl_SetVar2(interp, varName,NULL,
                    Tcl_DStringValue(&buffer), 0);

                goto configureDone;
            }
        }
    }

configureDone:
    if (infoPtr->unparsedObjc > 0) {
	while (infoPtr->unparsedObjc-- > 1) {
	    Tcl_DecrRefCount(infoPtr->unparsedObjv[infoPtr->unparsedObjc]);
	}
        ckfree ((char *)infoPtr->unparsedObjv);
        infoPtr->unparsedObjv = NULL;
        infoPtr->unparsedObjc = 0;
    }
    Tcl_DStringFree(&buffer2);
    Tcl_DStringFree(&buffer);

    return result;
}


/*
 * ------------------------------------------------------------------------
 *  Itcl_BiCgetCmd()
 *
 *  Invoked whenever the user issues the "cget" method for an object.
 *  Handles the following syntax:
 *
 *    <objName> cget -<option>
 *
 *  Allows access to public variables as if they were configuration
 *  options.  Mimics the behavior of the usual "cget" method for
 *  Tk widgets.  Returns the current value of the public variable
 *  with name <option>.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiCgetCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    Tcl_HashEntry *hPtr;
    ItclVarLookup *vlookup;
    const char *name;
    const char *val;
    int result;

    ItclShowArgs(1,"Itcl_BiCgetCmd", objc, objv);
    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((contextIoPtr == NULL) || objc != 2) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "improper usage: should be \"object cget -option\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  BE CAREFUL:  work in the virtual scope!
     */
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }

    if (!(contextIclsPtr->flags & ITCL_CLASS)) {
        result = ItclExtendedCget(contextIclsPtr, interp, objc, objv);
        if (result != TCL_CONTINUE) {
            return result;
        }
    }
    name = Tcl_GetString(objv[1]);

    vlookup = NULL;
    hPtr = ItclResolveVarEntry(contextIclsPtr, name+1);
    if (hPtr) {
        vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
    }

    if ((vlookup == NULL) || (vlookup->ivPtr->protection != ITCL_PUBLIC)) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "unknown option \"", name, "\"",
            NULL);
        return TCL_ERROR;
    }

    val = Itcl_GetInstanceVar(interp,
            Tcl_GetString(vlookup->ivPtr->namePtr),
            contextIoPtr, vlookup->ivPtr->iclsPtr);

    if (val) {
        Tcl_SetObjResult(interp, Tcl_NewStringObj(val, -1));
    } else {
        Tcl_SetObjResult(interp, Tcl_NewStringObj("<undefined>", -1));
    }
    return TCL_OK;
}


/*
 * ------------------------------------------------------------------------
 *  ItclReportPublicOpt()
 *
 *  Returns information about a public variable formatted as a
 *  configuration option:
 *
 *    -<varName> <initVal> <currentVal>
 *
 *  Used by Itcl_BiConfigureCmd() to report configuration options.
 *  Returns a Tcl_Obj containing the information.
 * ------------------------------------------------------------------------
 */
static Tcl_Obj*
ItclReportPublicOpt(
    Tcl_Interp *interp,      /* interpreter containing the object */
    ItclVariable *ivPtr,     /* public variable to be reported */
    ItclObject *contextIoPtr) /* object containing this variable */
{
    const char *val;
    ItclClass *iclsPtr;
    Tcl_HashEntry *hPtr;
    ItclVarLookup *vlookup;
    Tcl_DString optName;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;

    listPtr = Tcl_NewListObj(0, NULL);

    /*
     *  Determine how the option name should be reported.
     *  If the simple name can be used to find it in the virtual
     *  data table, then use the simple name.  Otherwise, this
     *  is a shadowed variable; use the full name.
     */
    Tcl_DStringInit(&optName);
    Tcl_DStringAppend(&optName, "-", -1);

    iclsPtr = (ItclClass*)contextIoPtr->iclsPtr;
    hPtr = ItclResolveVarEntry(iclsPtr,
            Tcl_GetString(ivPtr->fullNamePtr));
    assert(hPtr != NULL);
    vlookup = (ItclVarLookup*)Tcl_GetHashValue(hPtr);
    Tcl_DStringAppend(&optName, vlookup->leastQualName, -1);

    objPtr = Tcl_NewStringObj(Tcl_DStringValue(&optName), -1);
    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    Tcl_DStringFree(&optName);


    if (ivPtr->init) {
        objPtr = ivPtr->init;
    } else {
        objPtr = Tcl_NewStringObj("<undefined>", -1);
    }
    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);

    val = Itcl_GetInstanceVar(interp, Tcl_GetString(ivPtr->namePtr),
            contextIoPtr, ivPtr->iclsPtr);

    if (val) {
        objPtr = Tcl_NewStringObj((const char *)val, -1);
    } else {
        objPtr = Tcl_NewStringObj("<undefined>", -1);
    }
    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);

    return listPtr;
}

/*
 * ------------------------------------------------------------------------
 *  ItclReportOption()
 *
 *  Returns information about an option formatted as a
 *  configuration option:
 *
 *    <optionName> <initVal> <currentVal>
 *
 *  Used by ItclExtendedConfigure() to report configuration options.
 *  Returns a Tcl_Obj containing the information.
 * ------------------------------------------------------------------------
 */
static Tcl_Obj*
ItclReportOption(
    Tcl_Interp *interp,      /* interpreter containing the object */
    ItclOption *ioptPtr,     /* option to be reported */
    ItclObject *contextIoPtr) /* object containing this variable */
{
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    ItclDelegatedOption *idoPtr;
    const char *val;

    listPtr = Tcl_NewListObj(0, NULL);
    idoPtr = ioptPtr->iclsPtr->infoPtr->currIdoPtr;
    if (idoPtr != NULL) {
        Tcl_ListObjAppendElement(NULL, listPtr, idoPtr->namePtr);
	if (idoPtr->resourceNamePtr == NULL) {
            Tcl_ListObjAppendElement(NULL, listPtr,
                    Tcl_NewStringObj("", -1));
	    /* FIXME possible memory leak */
	} else {
            Tcl_ListObjAppendElement(NULL, listPtr,
                    idoPtr->resourceNamePtr);
	}
	if (idoPtr->classNamePtr == NULL) {
            Tcl_ListObjAppendElement(NULL, listPtr,
                    Tcl_NewStringObj("", -1));
	    /* FIXME possible memory leak */
	} else {
            Tcl_ListObjAppendElement(NULL, listPtr,
	            idoPtr->classNamePtr);
        }
    } else {
        Tcl_ListObjAppendElement(NULL, listPtr, ioptPtr->namePtr);
        Tcl_ListObjAppendElement(NULL, listPtr,
                ioptPtr->resourceNamePtr);
        Tcl_ListObjAppendElement(NULL, listPtr,
	        ioptPtr->classNamePtr);
    }
    if (ioptPtr->defaultValuePtr) {
        objPtr = ioptPtr->defaultValuePtr;
    } else {
        objPtr = Tcl_NewStringObj("<undefined>", -1);
    }
    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    val = ItclGetInstanceVar(interp, "itcl_options",
            Tcl_GetString(ioptPtr->namePtr),
            contextIoPtr, ioptPtr->iclsPtr);
    if (val) {
        objPtr = Tcl_NewStringObj((const char *)val, -1);
    } else {
        objPtr = Tcl_NewStringObj("<undefined>", -1);
    }
    Tcl_ListObjAppendElement(NULL, listPtr, objPtr);
    return listPtr;
}



/*
 * ------------------------------------------------------------------------
 *  Itcl_BiChainCmd()
 *
 *  Invoked to handle the "chain" command, to access the version of
 *  a method or proc that exists in a base class.  Handles the
 *  following syntax:
 *
 *    chain ?<arg> <arg>...?
 *
 *  Looks up the inheritance hierarchy for another implementation
 *  of the method/proc that is currently executing.  If another
 *  implementation is found, it is invoked with the specified
 *  <arg> arguments.  If it is not found, this command does nothing.
 *  This allows a base class method to be called out in a generic way,
 *  so the code will not have to change if the base class changes.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
NRBiChainCmd(
    void *dummy,        /* not used */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int result = TCL_OK;

    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    const char *cmd;
    char *cmd1;
    const char *head;
    ItclClass *iclsPtr;
    ItclHierIter hier;
    Tcl_HashEntry *hPtr;
    ItclMemberFunc *imPtr;
    Tcl_DString buffer;
    Tcl_Obj *cmdlinePtr;
    Tcl_Obj **newobjv;
    Tcl_Obj * const *cObjv;
    int cObjc;
    int idx;
    Tcl_Obj *objPtr;

    ItclShowArgs(1, "Itcl_BiChainCmd", objc, objv);

    /*
     *  If this command is not invoked within a class namespace,
     *  signal an error.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        Tcl_ResetResult(interp);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "cannot chain functions outside of a class context",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  Try to get the command name from the current call frame.
     *  If it cannot be determined, do nothing.  Otherwise, trim
     *  off any leading path names.
     */
    cObjv = Itcl_GetCallVarFrameObjv(interp);
    if (cObjv == NULL) {
        return TCL_OK;
    }
    cObjc = Itcl_GetCallVarFrameObjc(interp);

    if ((Itcl_GetCallFrameClientData(interp) == NULL) || (objc == 1)) {
        /* that has been a direct call, so no object in front !! */
        if (objc == 1 && cObjc >= 2) {
            idx = 1;
        } else {
            idx = 0;
        }
    } else {
	idx = 1;
    }
    cmd1 = (char *)ckalloc(strlen(Tcl_GetString(cObjv[idx]))+1);
    strcpy(cmd1, Tcl_GetString(cObjv[idx]));
    Itcl_ParseNamespPath(cmd1, &buffer, &head, &cmd);

    /*
     *  Look for the specified command in one of the base classes.
     *  If we have an object context, then start from the most-specific
     *  class and walk up the hierarchy to the current context.  If
     *  there is multiple inheritance, having the entire inheritance
     *  hierarchy will allow us to jump over to another branch of
     *  the inheritance tree.
     *
     *  If there is no object context, just start with the current
     *  class context.
     */
    if (contextIoPtr != NULL) {
        Itcl_InitHierIter(&hier, contextIoPtr->iclsPtr);
        while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
            if (iclsPtr == contextIclsPtr) {
                break;
            }
        }
    } else {
        Itcl_InitHierIter(&hier, contextIclsPtr);
        Itcl_AdvanceHierIter(&hier);    /* skip the current class */
    }

    /*
     *  Now search up the class hierarchy for the next implementation.
     *  If found, execute it.  Otherwise, do nothing.
     */
    objPtr = Tcl_NewStringObj(cmd, -1);
    ckfree(cmd1);
    Tcl_IncrRefCount(objPtr);
    while ((iclsPtr = Itcl_AdvanceHierIter(&hier)) != NULL) {
        hPtr = Tcl_FindHashEntry(&iclsPtr->functions, (char *)objPtr);
        if (hPtr) {
	    int my_objc;
            imPtr = (ItclMemberFunc*)Tcl_GetHashValue(hPtr);

            /*
             *  NOTE:  Avoid the usual "virtual" behavior of
             *         methods by passing the full name as
             *         the command argument.
             */

            cmdlinePtr = Itcl_CreateArgs(interp,
	            Tcl_GetString(imPtr->fullNamePtr), objc-1, objv+1);

            (void) Tcl_ListObjGetElements(NULL, cmdlinePtr,
                &my_objc, &newobjv);

	    if (imPtr->flags & ITCL_CONSTRUCTOR) {
		contextIoPtr = imPtr->iclsPtr->infoPtr->currIoPtr;
            }
            ItclShowArgs(1, "___chain", objc-1, newobjv+1);
            result = Itcl_EvalMemberCode(interp, imPtr, contextIoPtr,
	            my_objc-1, newobjv+1);
            Tcl_DecrRefCount(cmdlinePtr);
            break;
        }
    }
    Tcl_DecrRefCount(objPtr);

    Tcl_DStringFree(&buffer);
    Itcl_DeleteHierIter(&hier);
    return result;
}
/* ARGSUSED */
int
Itcl_BiChainCmd(
    void *clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return Tcl_NRCallObjProc(interp, NRBiChainCmd, clientData, objc, objv);
}

static int
CallCreateObject(
    void *data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_CallFrame frame;
    Tcl_Namespace *nsPtr;
    ItclClass *iclsPtr = (ItclClass *)data[0];
    int objc = PTR2INT(data[1]);
    Tcl_Obj *const *objv = (Tcl_Obj *const *)data[2];

    if (result != TCL_OK) {
        return result;
    }
    nsPtr = Itcl_GetUplevelNamespace(interp, 1);
    if (Itcl_PushCallFrame(interp, &frame, nsPtr,
            /*isProcCallFrame*/0) != TCL_OK) {
        return TCL_ERROR;
    }
    result = ItclClassCreateObject(iclsPtr->infoPtr, interp, objc, objv);
    Itcl_PopCallFrame(interp);
    Tcl_DecrRefCount(objv[2]);
    Tcl_DecrRefCount(objv[1]);
    Tcl_DecrRefCount(objv[0]);
    return result;
}

static int
PrepareCreateObject(
   Tcl_Interp *interp,
   ItclClass *iclsPtr,
   int objc,
   Tcl_Obj * const *objv)
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj **newObjv;
    void *callbackPtr;
    const char *funcName;
    int result;
    int offset;

    offset = 1;
    funcName = Tcl_GetString(objv[1]);
    if (strcmp(funcName, "itcl_hull") == 0) {
        hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)objv[1]);
	if (hPtr == NULL) {
	    Tcl_AppendResult(interp, "INTERNAL ERROR ",
		    "cannot find itcl_hull method", NULL);
	    return TCL_ERROR;
	}
	result = Itcl_ExecProc(Tcl_GetHashValue(hPtr), interp, objc, objv);
	return result;
    }
    if (strcmp(funcName, "create") == 0) {
        /* allow typeClassName create objectName */
        offset++;
    } else {
	/* allow typeClassName objectName */
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc+3-offset));
    newObjv[0] = objv[0];
    Tcl_IncrRefCount(newObjv[0]);
    newObjv[1] = iclsPtr->namePtr;
    Tcl_IncrRefCount(newObjv[1]);
    newObjv[2] = Tcl_NewStringObj(iclsPtr->nsPtr->fullName, -1);
    Tcl_IncrRefCount(newObjv[2]);
    memcpy(newObjv+3, objv+offset, (objc-offset) * sizeof(Tcl_Obj *));
    callbackPtr = Itcl_GetCurrentCallbackPtr(interp);
    ItclShowArgs(1, "CREATE", objc+3-offset, newObjv);
    Tcl_NRAddCallback(interp, CallCreateObject, iclsPtr,
            INT2PTR(objc+3-offset), newObjv, NULL);
    result = Itcl_NRRunCallbacks(interp, callbackPtr);
    if (result != TCL_OK) {
        if (iclsPtr->infoPtr->currIoPtr != NULL) {
            /* we are in a constructor call */
            if (iclsPtr->infoPtr->currIoPtr->hadConstructorError == 0) {
	        iclsPtr->infoPtr->currIoPtr->hadConstructorError = 1;
	    }
        }
    }
    ckfree((char *)newObjv);
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  ItclBiClassUnknownCmd()
 *
 *  Invoked to handle the "classunknown" command
 *  this is called whenever an object is called with an unknown method/proc
 *  following syntax:
 *
 *    classunknown <object> <methodname> ?<arg> <arg>...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ItclBiClassUnknownCmd(
    void *clientData,   /* ItclObjectInfo Ptr */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashEntry *hPtr2;
    Tcl_Obj **newObjv;
    Tcl_Obj **lObjv;
    Tcl_Obj *listPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *resPtr;
    Tcl_DString buffer;
    ItclClass *iclsPtr;
    ItclObjectInfo *infoPtr;
    ItclComponent *icPtr;
    ItclDelegatedFunction *idmPtr;
    ItclDelegatedFunction *idmPtr2;
    ItclDelegatedFunction *starIdmPtr;
    const char *resStr;
    const char *val;
    const char *funcName;
    int lObjc;
    int result;
    int offset;
    int useComponent;
    int isItclHull;
    int isTypeMethod;
    int isStar;
    int isNew;
    int idx;

    ItclShowArgs(1, "ItclBiClassUnknownCmd", objc, objv);
    listPtr = NULL;
    useComponent = 1;
    isStar = 0;
    isTypeMethod = 0;
    isItclHull = 0;
    starIdmPtr = NULL;
    infoPtr = (ItclObjectInfo *)clientData;
    hPtr = Tcl_FindHashEntry(&infoPtr->namespaceClasses,
            (char *)Tcl_GetCurrentNamespace(interp));
    if (hPtr == NULL) {
        Tcl_AppendResult(interp, "INTERNAL ERROR: ItclBiClassUnknownCmd ",
	        "cannot find class\n", NULL);
        return TCL_ERROR;
    }
    iclsPtr = (ItclClass *)Tcl_GetHashValue(hPtr);
    funcName = Tcl_GetString(objv[1]);
    if (strcmp(funcName, "create") == 0) {
        /* check if we have a user method create. If not, it is the builtin
	 * create method and we don't need to check for delegation
	 * and components with ITCL_COMPONENT_INHERIT
	 */
        hPtr = Tcl_FindHashEntry(&iclsPtr->resolveCmds, (char *)objv[1]);
	if (hPtr == NULL) {
            return PrepareCreateObject(interp, iclsPtr, objc, objv);
        }
    }
    if (strcmp(funcName, "itcl_hull") == 0) {
        isItclHull = 1;
    }
    if (!isItclHull) {
        FOREACH_HASH_VALUE(icPtr, &iclsPtr->components) {
            if (icPtr->flags & ITCL_COMPONENT_INHERIT) {
	        val = Tcl_GetVar2(interp, Tcl_GetString(icPtr->namePtr),
		        NULL, 0);
	        if ((val != NULL) && (strlen(val) > 0)) {
                    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc));
		    newObjv[0] = Tcl_NewStringObj(val, -1);
		    Tcl_IncrRefCount(newObjv[0]);
		    memcpy(newObjv+1, objv+1, sizeof(Tcl_Obj *) * (objc-1));
                    ItclShowArgs(1, "UK EVAL1", objc, newObjv);
                    result = Tcl_EvalObjv(interp, objc, newObjv, 0);
		    Tcl_DecrRefCount(newObjv[0]);
		    ckfree((char *)newObjv);
	            return result;
	        }
	    }
        }
    }
    /* from a class object only typemethods can be called directly
     * if delegated, so check for that, otherwise create an object
     * for ITCL_ECLASS we allow calling too
     */
    hPtr = NULL;
    isTypeMethod = 0;
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
        if (strcmp(Tcl_GetString(idmPtr->namePtr), funcName) == 0) {
            if (idmPtr->flags & ITCL_TYPE_METHOD) {
	       isTypeMethod = 1;
	    }
	    if (iclsPtr->flags & ITCL_ECLASS) {
	       isTypeMethod = 1;
	    }
	    break;
        }
        if (strcmp(Tcl_GetString(idmPtr->namePtr), "*") == 0) {
            if (idmPtr->flags & ITCL_TYPE_METHOD) {
	       isTypeMethod = 1;
	    }
	    starIdmPtr = idmPtr;
	    break;
	}
    }
    idmPtr = NULL;
    if (isTypeMethod) {
	hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions, (char *)objv[1]);
	if (hPtr == NULL) {
	    objPtr = Tcl_NewStringObj("*", -1);
	    Tcl_IncrRefCount(objPtr);
	    hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions,
	            (char *)objPtr);
	    Tcl_DecrRefCount(objPtr);
	    if (hPtr != NULL) {
	        idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
	        isStar = 1;
	    }
	}
	if (isStar) {
            /* check if the function is in the exceptions */
	    hPtr2 = Tcl_FindHashEntry(&starIdmPtr->exceptions, (char *)objv[1]);
	    if (hPtr2 != NULL) {
		const char *sep = "";
		objPtr = Tcl_NewStringObj("unknown subcommand \"", -1);
		Tcl_AppendToObj(objPtr, funcName, -1);
		Tcl_AppendToObj(objPtr, "\": must be ", -1);
		FOREACH_HASH_VALUE(idmPtr,
			&iclsPtr->delegatedFunctions) {
		    funcName = Tcl_GetString(idmPtr->namePtr);
		    if (strcmp(funcName, "*") != 0) {
			if (strlen(sep) > 0) {
		            Tcl_AppendToObj(objPtr, sep, -1);
			}
		        Tcl_AppendToObj(objPtr, funcName, -1);
			sep = " or ";
		    }
		}
	        Tcl_SetObjResult(interp, objPtr);
		return TCL_ERROR;
	    }
	}
	if (hPtr != NULL) {
	    idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
	    val = NULL;
	    if (idmPtr->icPtr != NULL) {
                if (idmPtr->icPtr->ivPtr->flags & ITCL_COMMON) {
	            val = Tcl_GetVar2(interp,
	                    Tcl_GetString(idmPtr->icPtr->namePtr), NULL, 0);
		} else {
                    ItclClass *contextIclsPtr;
                    ItclObject *contextIoPtr;
                    contextIclsPtr = NULL;
                    contextIoPtr = NULL;
                    Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr);
                    Tcl_DStringInit(&buffer);
                    Tcl_DStringAppend(&buffer,
                            Tcl_GetString(contextIoPtr->varNsNamePtr), -1);
                    Tcl_DStringAppend(&buffer,
                            Tcl_GetString(idmPtr->icPtr->ivPtr->fullNamePtr),
                            -1);
                    val = Tcl_GetVar2(interp, Tcl_DStringValue(&buffer),
                            NULL, 0);
		    Tcl_DStringFree(&buffer);
		}
	        if (val == NULL) {
                    Tcl_AppendResult(interp, "INTERNAL ERROR: ",
		            "ItclBiClassUnknownCmd contents ",
		            "of component == NULL\n", NULL);
	            return TCL_ERROR;
	        }
	    }
	    offset = 1;
	    lObjc = 0;
	    if ((idmPtr->asPtr != NULL) || (idmPtr->usingPtr != NULL)) {
		offset++;
                listPtr = Tcl_NewListObj(0, NULL);
                result = ExpandDelegateAs(interp, NULL, iclsPtr,
		        idmPtr, funcName, listPtr);
                if (result != TCL_OK) {
                    return result;
                }
	        result = Tcl_ListObjGetElements(interp, listPtr,
		        &lObjc, &lObjv);
	        if (result != TCL_OK) {
		    Tcl_DecrRefCount(listPtr);
		    return result;
		}
		if (idmPtr->usingPtr != NULL) {
                    useComponent = 0;
		}
	    }
	    if (useComponent) {
	        if ((val == NULL) || (strlen(val) == 0)) {
		    Tcl_AppendResult(interp, "component \"",
		            Tcl_GetString(idmPtr->icPtr->namePtr),
			    "\" is not initialized", NULL);
		    return TCL_ERROR;
	        }
	    }
            newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) *
	            (objc + lObjc - offset + useComponent));
	    if (useComponent) {
	        newObjv[0] = Tcl_NewStringObj(val, -1);
	        Tcl_IncrRefCount(newObjv[0]);
	    }
	    for (idx = 0; idx < lObjc; idx++) {
		newObjv[useComponent+idx] = lObjv[idx];
	    }
	    if (objc-offset > 0) {
                memcpy(newObjv+useComponent+lObjc, objv+offset,
	                sizeof(Tcl_Obj *) * (objc-offset));
            }
	    ItclShowArgs(1, "OBJ UK EVAL", objc+lObjc-offset+useComponent,
	            newObjv);
            result = Tcl_EvalObjv(interp,
		    objc+lObjc-offset+useComponent, newObjv, 0);
            if (isStar && (result == TCL_OK)) {
		if (Tcl_FindHashEntry(&iclsPtr->delegatedFunctions,
		        (char *)newObjv[1]) == NULL) {
                    result = ItclCreateDelegatedFunction(interp, iclsPtr,
		            newObjv[1], idmPtr->icPtr, NULL, NULL,
			    NULL, &idmPtr2);
		    if (result == TCL_OK) {
			if (isTypeMethod) {
			    idmPtr2->flags |= ITCL_TYPE_METHOD;
		        } else {
		            idmPtr2->flags |= ITCL_METHOD;
			}
		        hPtr2 = Tcl_CreateHashEntry(
			        &iclsPtr->delegatedFunctions,
				(char *)newObjv[1], &isNew);
                        Tcl_SetHashValue(hPtr2, idmPtr2);
		    }
		}
	    }
	    if (useComponent) {
	        Tcl_DecrRefCount(newObjv[0]);
	    }
	    ckfree((char *)newObjv);
	    if (listPtr != NULL) {
		Tcl_DecrRefCount(listPtr);
	    }
	    if (result == TCL_ERROR) {
		resStr = Tcl_GetString(Tcl_GetObjResult(interp));
		/* FIXME ugly hack at the moment !! */
		if (strncmp(resStr, "wrong # args: should be ", 24) == 0) {
		    resPtr = Tcl_NewStringObj("", -1);
		    Tcl_AppendToObj(resPtr, resStr, 25);
                    resStr += 25;
		    Tcl_AppendToObj(resPtr, Tcl_GetString(iclsPtr->namePtr),
		           -1);
                    resStr += strlen(val);
		    Tcl_AppendToObj(resPtr, resStr, -1);
		    Tcl_ResetResult(interp);
		    Tcl_SetObjResult(interp, resPtr);
		}
	    }
	    return result;
	}
    }
    return PrepareCreateObject(interp, iclsPtr, objc, objv);
}

/*
 * ------------------------------------------------------------------------
 *  ItclUnknownGuts()
 *
 *  The unknown method handler of the itcl::Root class -- all Itcl
 *  objects land here when they cannot find a method.
 *
 * ------------------------------------------------------------------------
 */

int
ItclUnknownGuts(
    ItclObject *ioPtr,	     /* The ItclObject seeking method */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashEntry *hPtr2;
    Tcl_Obj **newObjv;
    Tcl_Obj **lObjv;
    Tcl_Obj *listPtr = NULL;
    Tcl_Obj *objPtr;
    Tcl_Obj *resPtr;
    Tcl_DString buffer;
    ItclClass *iclsPtr;
    ItclComponent *icPtr;
    ItclDelegatedFunction *idmPtr;
    ItclDelegatedFunction *idmPtr2;
    const char *resStr;
    const char *val;
    const char *funcName;
    int lObjc;
    int result;
    int offset;
    int useComponent;
    int found;
    int isItclHull;
    int isStar;
    int isTypeMethod;
    int isNew;
    int idx;

    if (objc < 2) {
        Tcl_AppendResult(interp, "wrong # args: should be one of...",
		NULL);
        ItclReportObjectUsage(interp, ioPtr, NULL, NULL);
        return TCL_ERROR;
    }
    iclsPtr = ioPtr->iclsPtr;
    lObjc = 0;
    offset = 1;
    isStar = 0;
    found = 0;
    isItclHull = 0;
    useComponent = 1;
    result = TCL_OK;
    idmPtr = NULL;
    funcName = Tcl_GetString(objv[1]);
    if (strcmp(funcName, "itcl_hull") == 0) {
        isItclHull = 1;
    }
    icPtr = NULL;
    if (!isItclHull) {
        FOREACH_HASH_VALUE(icPtr, &ioPtr->objectComponents) {
            if (icPtr->flags & ITCL_COMPONENT_INHERIT) {
	        val = Itcl_GetInstanceVar(interp,
	                Tcl_GetString(icPtr->namePtr), ioPtr,
		        icPtr->ivPtr->iclsPtr);
	        if ((val != NULL) && (strlen(val) > 0)) {
                    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) *
		            (objc));
		    newObjv[0] = Tcl_NewStringObj(val, -1);
		    Tcl_IncrRefCount(newObjv[0]);
		    memcpy(newObjv+1, objv+1, sizeof(Tcl_Obj *) * (objc-1));
                    result = Tcl_EvalObjv(interp, objc, newObjv, 0);
		    Tcl_DecrRefCount(newObjv[0]);
		    ckfree((char *)newObjv);
	            return result;
	        }
	    }
        }
    }
    isTypeMethod = 0;
    found = 0;
    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
        if (strcmp(Tcl_GetString(idmPtr->namePtr), funcName) == 0) {
            if (idmPtr->flags & ITCL_TYPE_METHOD) {
	       isTypeMethod = 1;
	    }
	    found = 1;
	    break;
        }
        if (strcmp(Tcl_GetString(idmPtr->namePtr), "*") == 0) {
            if (idmPtr->flags & ITCL_TYPE_METHOD) {
	       isTypeMethod = 1;
	    }
	    found = 1;
	    break;
	}
    }
    if (! found) {
        idmPtr = NULL;
    }
    iclsPtr = ioPtr->iclsPtr;
    found = 0;
    hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions, (char *)objv[1]);
    if (hPtr == NULL) {
        objPtr = Tcl_NewStringObj("*", -1);
        Tcl_IncrRefCount(objPtr);
        hPtr = Tcl_FindHashEntry(&iclsPtr->delegatedFunctions,
                (char *)objPtr);
        Tcl_DecrRefCount(objPtr);
	if (hPtr != NULL) {
	    idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
            isStar = 1;
	}
    } else {
	found = 1;
	idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
    }
    if (isStar) {
       /* check if the function is in the exceptions */
        hPtr2 = Tcl_FindHashEntry(&idmPtr->exceptions, (char *)objv[1]);
        if (hPtr2 != NULL) {
	    const char *sep = "";
	    objPtr = Tcl_NewStringObj("unknown subcommand \"", -1);
	    Tcl_AppendToObj(objPtr, funcName, -1);
	    Tcl_AppendToObj(objPtr, "\": must be ", -1);
	    FOREACH_HASH_VALUE(idmPtr,
		    &iclsPtr->delegatedFunctions) {
	        funcName = Tcl_GetString(idmPtr->namePtr);
	        if (strcmp(funcName, "*") != 0) {
		    if (strlen(sep) > 0) {
	                Tcl_AppendToObj(objPtr, sep, -1);
		    }
	            Tcl_AppendToObj(objPtr, funcName, -1);
		    sep = " or ";
	        }
	    }
            Tcl_SetObjResult(interp, objPtr);
	    return TCL_ERROR;
        }
    }
    val = NULL;
    if ((idmPtr != NULL) && (idmPtr->icPtr != NULL)) {
        Tcl_Obj *objPtr;
        /* we cannot use Itcl_GetInstanceVar here as the object is not
         * yet completely built. So use the varNsNamePtr
         */
        if (idmPtr->icPtr->ivPtr->flags & ITCL_COMMON) {
            objPtr = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE, -1);
            Tcl_AppendToObj(objPtr,
		    (Tcl_GetObjectNamespace(iclsPtr->oPtr))->fullName, -1);
            Tcl_AppendToObj(objPtr, "::", -1);
            Tcl_AppendToObj(objPtr,
	            Tcl_GetString(idmPtr->icPtr->namePtr), -1);
            val = Tcl_GetVar2(interp, Tcl_GetString(objPtr), NULL, 0);
	    Tcl_DecrRefCount(objPtr);
        } else {
            Tcl_DStringInit(&buffer);
            Tcl_DStringAppend(&buffer,
                    Tcl_GetString(ioPtr->varNsNamePtr), -1);
            Tcl_DStringAppend(&buffer,
                    Tcl_GetString(idmPtr->icPtr->ivPtr->fullNamePtr), -1);
            val = Tcl_GetVar2(interp, Tcl_DStringValue(&buffer),
                    NULL, 0);
	    Tcl_DStringFree(&buffer);
        }

        if (val == NULL) {
            Tcl_AppendResult(interp, "ItclBiObjectUnknownCmd contents of ",
	            "component == NULL\n", NULL);
            return TCL_ERROR;
        }
    }

    offset = 1;
    if (isStar) {
        hPtr = Tcl_FindHashEntry(&idmPtr->exceptions, (char *)objv[1]);
	/* we have no method name in that case in the caller */
	if (hPtr != NULL) {
	    const char *sep = "";
	    objPtr = Tcl_NewStringObj("unknown subcommand \"", -1);
	    Tcl_AppendToObj(objPtr, funcName, -1);
	    Tcl_AppendToObj(objPtr, "\": must be ", -1);
	    FOREACH_HASH_VALUE(idmPtr, &iclsPtr->delegatedFunctions) {
		funcName = Tcl_GetString(idmPtr->namePtr);
	        if (strcmp(funcName, "*") != 0) {
		    if (strlen(sep) > 0) {
	                Tcl_AppendToObj(objPtr, sep, -1);
		    }
	            Tcl_AppendToObj(objPtr, funcName, -1);
		    sep = " or ";
	        }
	    }
	}
    }
    if (idmPtr == NULL) {
        Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[1]),
                "\": should be one of...", NULL);
        ItclReportObjectUsage(interp, ioPtr, NULL, NULL);
        return TCL_ERROR;
    }
    lObjc = 0;
    if ((idmPtr != NULL) && ((idmPtr->asPtr != NULL) ||
            (idmPtr->usingPtr != NULL))) {
	offset++;
        listPtr = Tcl_NewListObj(0, NULL);
        result = ExpandDelegateAs(interp, NULL, iclsPtr,
		idmPtr, funcName, listPtr);
        if (result != TCL_OK) {
	    Tcl_DecrRefCount(listPtr);
            return result;
        }
        result = Tcl_ListObjGetElements(interp, listPtr,
	        &lObjc, &lObjv);
        if (result != TCL_OK) {
	    Tcl_DecrRefCount(listPtr);
	    return result;
	}
	if (idmPtr->usingPtr != NULL) {
            useComponent = 0;
	}
    }
    if (useComponent) {
	if ((val == NULL) || (strlen(val) == 0)) {
	    Tcl_AppendResult(interp, "component \"",
		    Tcl_GetString(idmPtr->icPtr->namePtr),
		    "\" is not initialized", NULL);
	    return TCL_ERROR;
        }
    }
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) *
                (objc + lObjc - offset + useComponent));
    if (useComponent) {
        newObjv[0] = Tcl_NewStringObj(val, -1);
        Tcl_IncrRefCount(newObjv[0]);
    }
    for (idx = 0; idx < lObjc; idx++) {
	newObjv[useComponent+idx] = lObjv[idx];
    }
    if (objc-offset > 0) {
        memcpy(newObjv+useComponent+lObjc, objv+offset,
                sizeof(Tcl_Obj *) * (objc-offset));
    }
    ItclShowArgs(1, "UK EVAL2", objc+lObjc-offset+useComponent,
            newObjv);
    result = Tcl_EvalObjv(interp, objc+lObjc-offset+useComponent,
            newObjv, 0);
    if (isStar && (result == TCL_OK)) {
	if (Tcl_FindHashEntry(&iclsPtr->delegatedFunctions,
	        (char *)newObjv[1]) == NULL) {
            result = ItclCreateDelegatedFunction(interp, iclsPtr,
	            newObjv[1], idmPtr->icPtr, NULL, NULL,
		    NULL, &idmPtr2);
	    if (result == TCL_OK) {
		if (isTypeMethod) {
		    idmPtr2->flags |= ITCL_TYPE_METHOD;
		} else {
		    idmPtr2->flags |= ITCL_METHOD;
		}
	        hPtr2 = Tcl_CreateHashEntry(
		        &iclsPtr->delegatedFunctions, (char *)newObjv[1],
			&isNew);
                Tcl_SetHashValue(hPtr2, idmPtr2);
	    }
	}
    }
    if (useComponent) {
        Tcl_DecrRefCount(newObjv[0]);
    }
    if (listPtr != NULL) {
        Tcl_DecrRefCount(listPtr);
    }
    ckfree((char *)newObjv);
    if (result == TCL_OK) {
        return TCL_OK;
    }
    resStr = Tcl_GetString(Tcl_GetObjResult(interp));
    /* FIXME ugly hack at the moment !! */
    if (strncmp(resStr, "wrong # args: should be ", 24) == 0) {
        resPtr = Tcl_NewStringObj("", -1);
	Tcl_AppendToObj(resPtr, resStr, 25);
        resStr += 25;
	Tcl_AppendToObj(resPtr, Tcl_GetString(iclsPtr->namePtr), -1);
        resStr += strlen(val);
	Tcl_AppendToObj(resPtr, resStr, -1);
	Tcl_ResetResult(interp);
	Tcl_SetObjResult(interp, resPtr);
    }
    return result;
}

static Tcl_Obj *makeAsOptionInfo(
    Tcl_Interp *interp,
    Tcl_Obj *optNamePtr,
    ItclDelegatedOption *idoPtr,
    int lObjc2,
    Tcl_Obj * const *lObjv2)
{
    Tcl_Obj *objPtr;
    int j;

    objPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj(
            Tcl_GetString(optNamePtr), -1));
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj(
            Tcl_GetString(idoPtr->resourceNamePtr), -1));
    Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj(
	    Tcl_GetString(idoPtr->classNamePtr), -1));
    for (j = 3; j < lObjc2; j++) {
         Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj(
		Tcl_GetString(lObjv2[j]), -1));
    }
    return objPtr;
}

/*
 * ------------------------------------------------------------------------
 *  ItclExtendedConfigure()
 *
 *  Invoked whenever the user issues the "configure" method for an object.
 *  If the class is not ITCL_CLASS
 *  Handles the following syntax:
 *
 *    <objName> configure ?-<option>? ?<value> -<option> <value>...?
 *
 *  Allows access to public variables as if they were configuration
 *  options.  With no arguments, this command returns the current
 *  list of public variable options.  If -<option> is specified,
 *  this returns the information for just one option:
 *
 *    -<optionName> <initVal> <currentVal>
 *
 *  Otherwise, the list of arguments is parsed, and values are
 *  assigned to the various public variable options.  When each
 *  option changes, a big of "config" code associated with the option
 *  is executed, to bring the object up to date.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ItclExtendedConfigure(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_HashTable unique;
    Tcl_HashEntry *hPtr2;
    Tcl_HashEntry *hPtr3;
    Tcl_Object oPtr;
    Tcl_Obj *listPtr;
    Tcl_Obj *listPtr2;
    Tcl_Obj *resultPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj *optNamePtr;
    Tcl_Obj *methodNamePtr;
    Tcl_Obj *configureMethodPtr;
    Tcl_Obj **lObjv;
    Tcl_Obj **newObjv;
    Tcl_Obj *lObjvOne[1];
    Tcl_Obj **lObjv2;
    Tcl_Obj **lObjv3;
    Tcl_Namespace *saveNsPtr;
    Tcl_Namespace *evalNsPtr;
    ItclClass *contextIclsPtr;
    ItclClass *iclsPtr2;
    ItclComponent *componentIcPtr;
    ItclObject *contextIoPtr;
    ItclDelegatedFunction *idmPtr;
    ItclDelegatedOption *idoPtr;
    ItclDelegatedOption *saveIdoPtr;
    ItclObject *ioPtr;
    ItclComponent *icPtr;
    ItclOption *ioptPtr;
    ItclObjectInfo *infoPtr;
    const char *val;
    int lObjc;
    int lObjc2;
    int lObjc3;
    int i;
    int j;
    int isNew;
    int result;
	    int isOneOption;

    ItclShowArgs(1, "ItclExtendedConfigure", objc, objv);
    ioptPtr = NULL;
    optNamePtr = NULL;
    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (contextIoPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "improper usage: should be ",
            "\"object configure ?-option? ?value -option value...?\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  BE CAREFUL:  work in the virtual scope!
     */
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    infoPtr = contextIclsPtr->infoPtr;
    if (infoPtr->currContextIclsPtr != NULL) {
        contextIclsPtr = infoPtr->currContextIclsPtr;
    }

    hPtr = NULL;
    /* first check if method configure is delegated */
    methodNamePtr = Tcl_NewStringObj("*", -1);
    hPtr = Tcl_FindHashEntry(&contextIclsPtr->delegatedFunctions, (char *)
            methodNamePtr);
    if (hPtr != NULL) {
	/* all methods are delegated */
        idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
	Tcl_SetStringObj(methodNamePtr, "configure", -1);
        hPtr = Tcl_FindHashEntry(&idmPtr->exceptions, (char *)methodNamePtr);
        if (hPtr == NULL) {
	    icPtr = idmPtr->icPtr;
	    val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr),
	            NULL, contextIoPtr, contextIclsPtr);
            if (val != NULL) {
	        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+5));
	        newObjv[0] = Tcl_NewStringObj(val, -1);
	        Tcl_IncrRefCount(newObjv[0]);
	        newObjv[1] = Tcl_NewStringObj("configure", -1);
	        Tcl_IncrRefCount(newObjv[1]);
	        for(i=1;i<objc;i++) {
	            newObjv[i+1] = objv[i];
                }
		objPtr = Tcl_NewStringObj(val, -1);
	        Tcl_IncrRefCount(objPtr);
	        oPtr = Tcl_GetObjectFromObj(interp, objPtr);
	        if (oPtr != NULL) {
                    ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
                            infoPtr->object_meta_type);
	            infoPtr->currContextIclsPtr = ioPtr->iclsPtr;
	        }
		ItclShowArgs(1, "EXTENDED CONFIGURE EVAL1", objc+1, newObjv);
                result = Tcl_EvalObjv(interp, objc+1, newObjv, TCL_EVAL_DIRECT);
                Tcl_DecrRefCount(newObjv[0]);
                Tcl_DecrRefCount(newObjv[1]);
                ckfree((char *)newObjv);
	        Tcl_DecrRefCount(objPtr);
	        if (oPtr != NULL) {
	            infoPtr->currContextIclsPtr = NULL;
	        }
		Tcl_DecrRefCount(methodNamePtr);
                return result;
	    }
	} else {
	    /* configure is not delegated, so reset hPtr for checks later on! */
	    hPtr = NULL;
	}
    }
    Tcl_DecrRefCount(methodNamePtr);
    /* now do the hard work */
    if (objc == 1) {
	Tcl_InitObjHashTable(&unique);
	/* plain configure */
        listPtr = Tcl_NewListObj(0, NULL);
	if (contextIclsPtr->flags & ITCL_ECLASS) {
            result = Tcl_EvalEx(interp, "::itcl::builtin::getEclassOptions", -1, 0);
            return result;
	}
	FOREACH_HASH_VALUE(ioptPtr, &contextIoPtr->objectOptions) {
	    hPtr2 = Tcl_CreateHashEntry(&unique,
	            (char *)ioptPtr->namePtr, &isNew);
	    if (!isNew) {
	        continue;
	    }
	    objPtr = Tcl_NewListObj(0, NULL);
	    Tcl_ListObjAppendElement(interp, objPtr,
	            Tcl_NewStringObj(Tcl_GetString(ioptPtr->namePtr), -1));
	    Tcl_ListObjAppendElement(interp, objPtr,
	            Tcl_NewStringObj(
		    Tcl_GetString(ioptPtr->resourceNamePtr), -1));
	    Tcl_ListObjAppendElement(interp, objPtr,
	            Tcl_NewStringObj(Tcl_GetString(ioptPtr->classNamePtr), -1));
	    if (ioptPtr->defaultValuePtr != NULL) {
	        Tcl_ListObjAppendElement(interp, objPtr, Tcl_NewStringObj(
		        Tcl_GetString(ioptPtr->defaultValuePtr), -1));
	    } else {
	        Tcl_ListObjAppendElement(interp, objPtr,
		        Tcl_NewStringObj("", -1));
	    }
	    val = ItclGetInstanceVar(interp, "itcl_options",
	            Tcl_GetString(ioptPtr->namePtr), contextIoPtr,
		    contextIclsPtr);
	    if (val == NULL) {
		val = "<undefined>";
	    }
	    Tcl_ListObjAppendElement(interp, objPtr,
	            Tcl_NewStringObj(val, -1));
	    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	}
	/* now check for delegated options */
	FOREACH_HASH_VALUE(idoPtr, &contextIoPtr->objectDelegatedOptions) {

            if (idoPtr->icPtr != NULL) {
                icPtr = idoPtr->icPtr;
                val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr),
                    NULL, contextIoPtr, icPtr->ivPtr->iclsPtr);
	        if ((val != NULL) && (strlen(val) != 0)) {

		    objPtr = Tcl_NewStringObj(val, -1);
		    Tcl_IncrRefCount(objPtr);
		    Tcl_AppendToObj(objPtr, " configure ", -1);
		    isOneOption = 0;
		    if (strcmp(Tcl_GetString(idoPtr->namePtr), "*") != 0) {
		        Tcl_AppendToObj(objPtr, " ", -1);
			if (idoPtr->asPtr != NULL) {
		            Tcl_AppendToObj(objPtr, Tcl_GetString(
			            idoPtr->asPtr), -1);
			} else {
		            Tcl_AppendToObj(objPtr, Tcl_GetString(
			            idoPtr->namePtr), -1);
			}
		        isOneOption = 1;
		    }
                    result = Tcl_EvalObjEx(interp, objPtr, 0);
		    Tcl_DecrRefCount(objPtr);
                    if (result != TCL_OK) {
		        return TCL_ERROR;
		    }
		    listPtr2 = Tcl_GetObjResult(interp);
		    if (isOneOption) {
		        lObjc = 1;
			lObjvOne[0] = listPtr2;
                        lObjv = &lObjvOne[0];
		    } else {
		        Tcl_ListObjGetElements(interp, listPtr2,
		                &lObjc, &lObjv);
                    }
		    for (i = 0; i < lObjc; i++) {
			objPtr = lObjv[i];
		        Tcl_ListObjGetElements(interp, objPtr,
		            &lObjc2, &lObjv2);
			optNamePtr = idoPtr->namePtr;
			if (lObjc2 == 0) {
			    hPtr = NULL;
			} else {
			    hPtr = Tcl_FindHashEntry(&idoPtr->exceptions,
			            (char *)lObjv2[0]);
			    if (isOneOption) {
				/* avoid wrong name where asPtr != NULL */
			        optNamePtr = idoPtr->namePtr;
                            } else {
			        optNamePtr = lObjv2[0];
			    }
			}
			if ((hPtr == NULL) && (lObjc2 > 0)) {
			    if (icPtr->haveKeptOptions) {
			        hPtr = Tcl_FindHashEntry(&icPtr->keptOptions,
				        (char *)optNamePtr);
				if (hPtr == NULL) {
				   if (idoPtr->asPtr != NULL) {
				       if (strcmp(Tcl_GetString(idoPtr->asPtr),
				               Tcl_GetString(lObjv2[0])) == 0) {
					   hPtr = Tcl_FindHashEntry(
					        &icPtr->keptOptions,
					        (char *)optNamePtr);
				           if (hPtr == NULL) {
				               /* not in kept list, so ignore */
				               continue;
				           }
				           objPtr = makeAsOptionInfo(interp,
					       optNamePtr, idoPtr, lObjc2,
					       lObjv2);
			               }
			            }
			        }
				if (hPtr != NULL) {
	                            hPtr2 = Tcl_CreateHashEntry(&unique,
	                                    (char *)optNamePtr, &isNew);
	                            if (!isNew) {
	                                continue;
	                            }
			            /* add the option */
				    if (idoPtr->asPtr != NULL) {
				        objPtr = makeAsOptionInfo(interp,
				                optNamePtr, idoPtr, lObjc2,
					        lObjv2);
				    }
	                            Tcl_ListObjAppendElement(interp, listPtr,
				            objPtr);
			        }
			    } else {
		                Tcl_ListObjGetElements(interp, lObjv2[i],
		                        &lObjc3, &lObjv3);
	                        hPtr2 = Tcl_CreateHashEntry(&unique,
	                                (char *)lObjv3[0], &isNew);
	                        if (!isNew) {
	                            continue;
	                        }
			        /* add the option */
				if (idoPtr->asPtr != NULL) {
				    objPtr = makeAsOptionInfo(interp,
				            optNamePtr, idoPtr, lObjc2,
				            lObjv2);
				}
	                        Tcl_ListObjAppendElement(interp, listPtr,
				    objPtr);
			    }
		        }
		    }
		}
	    }
	}
	Tcl_SetObjResult(interp, listPtr);
	Tcl_DeleteHashTable(&unique);
        return TCL_OK;
    }
    hPtr2 = NULL;
    /* first handle delegated options */
    hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedOptions, (char *)
            objv[1]);
    if (hPtr == NULL) {
	Tcl_Obj *objPtr;
	objPtr = Tcl_NewStringObj("*",1);
	Tcl_IncrRefCount(objPtr);
        /* check if all options are delegated */
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedOptions,
	        (char *)objPtr);
	Tcl_DecrRefCount(objPtr);
        if (hPtr != NULL) {
	    /* now check the exceptions */
            idoPtr = (ItclDelegatedOption *)Tcl_GetHashValue(hPtr);
	    hPtr2 = Tcl_FindHashEntry(&idoPtr->exceptions, (char *)objv[1]);
	    if (hPtr2 != NULL) {
		/* found in exceptions, so no delegation for this option */
	        hPtr = NULL;
	    }
        }
    }
    componentIcPtr = NULL;
    /* check if it is not a local option defined before delegate option "*"
     */
    hPtr2 = Tcl_FindHashEntry(&contextIoPtr->objectOptions,
            (char *)objv[1]);
    if (hPtr != NULL) {
        idoPtr = (ItclDelegatedOption *)Tcl_GetHashValue(hPtr);
        icPtr = idoPtr->icPtr;
        if (icPtr != NULL) {
	    if (icPtr->haveKeptOptions) {
	        hPtr3 = Tcl_FindHashEntry(&icPtr->keptOptions, (char *)objv[1]);
                if (hPtr3 != NULL) {
		    /* ignore if it is an object option only */
		    ItclHierIter hier;
		    int found;

		    found = 0;
                    Itcl_InitHierIter(&hier, contextIoPtr->iclsPtr);
		    iclsPtr2 = Itcl_AdvanceHierIter(&hier);
		    while (iclsPtr2 != NULL) {
			if (Tcl_FindHashEntry(&iclsPtr2->options,
			        (char *)objv[1]) != NULL) {
                            found = 1;
			    break;
			}
                        iclsPtr2 = Itcl_AdvanceHierIter(&hier);
		    }
		    Itcl_DeleteHierIter(&hier);
                    if (! found) {
		        hPtr2 = NULL;
                        componentIcPtr = icPtr;
		    }
	        }
	    }
	}
    }
    if ((objc <= 3) && (hPtr != NULL) && (hPtr2 == NULL)) {
	/* the option is delegated */
        idoPtr = (ItclDelegatedOption *)Tcl_GetHashValue(hPtr);
	if (componentIcPtr != NULL) {
	    icPtr = componentIcPtr;
	} else {
            icPtr = idoPtr->icPtr;
	}
        val = ItclGetInstanceVar(interp,
	        Tcl_GetString(icPtr->namePtr),
                NULL, contextIoPtr, icPtr->ivPtr->iclsPtr);
        if ((val != NULL) && (strlen(val) > 0)) {
	    if (idoPtr->asPtr != NULL) {
                icPtr->ivPtr->iclsPtr->infoPtr->currIdoPtr = idoPtr;
	    }
	    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+2));
	    newObjv[0] = Tcl_NewStringObj(val, -1);
	    Tcl_IncrRefCount(newObjv[0]);
	    newObjv[1] = Tcl_NewStringObj("configure", 9);
	    Tcl_IncrRefCount(newObjv[1]);
	    if (idoPtr->asPtr != NULL) {
	        newObjv[2] = idoPtr->asPtr;
	    } else {
	        newObjv[2] = objv[1];
	    }
	    Tcl_IncrRefCount(newObjv[2]);
	    for(i=2;i<objc;i++) {
	        newObjv[i+1] = objv[i];
            }
	    objPtr = Tcl_NewStringObj(val, -1);
	    Tcl_IncrRefCount(objPtr);
	    oPtr = Tcl_GetObjectFromObj(interp, objPtr);
	    if (oPtr != NULL) {
                ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
                        infoPtr->object_meta_type);
	        infoPtr->currContextIclsPtr = ioPtr->iclsPtr;
	    }
	    Tcl_DecrRefCount(objPtr);
            ItclShowArgs(1, "extended eval delegated option", objc + 1,
	            newObjv);
            result = Tcl_EvalObjv(interp, objc+1, newObjv, TCL_EVAL_DIRECT);
	    Tcl_DecrRefCount(newObjv[2]);
            Tcl_DecrRefCount(newObjv[1]);
            Tcl_DecrRefCount(newObjv[0]);
            ckfree((char *)newObjv);
            icPtr->ivPtr->iclsPtr->infoPtr->currIdoPtr = NULL;
	    if (oPtr != NULL) {
	        infoPtr->currContextIclsPtr = NULL;
	    }
            return result;
        } else {
	    Tcl_AppendResult(interp, "INTERNAL ERROR component \"",
	            Tcl_GetString(icPtr->namePtr), "\" not found",
	            " or not set in ItclExtendedConfigure delegated option",
		    NULL);
	    return TCL_ERROR;
	}
    }

    if (objc == 2) {
	saveIdoPtr = infoPtr->currIdoPtr;
        /* now look if it is an option at all */
	if (hPtr2 == NULL) {
            hPtr2 = Tcl_FindHashEntry(&contextIclsPtr->options,
	            (char *) objv[1]);
            if (hPtr2 == NULL) {
                hPtr2 = Tcl_FindHashEntry(&contextIoPtr->objectOptions,
	                (char *) objv[1]);
	    } else {
	       infoPtr->currIdoPtr = NULL;
	    }
	}
        if (hPtr2 == NULL) {
            if (contextIclsPtr->flags & ITCL_ECLASS) {
		newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc));
                newObjv[0] = Tcl_NewStringObj("::itcl::builtin::eclassConfigure", -1);
		Tcl_IncrRefCount(newObjv[0]);
		for (j = 1; j < objc; j++) {
                    newObjv[j] = objv[j];
		    Tcl_IncrRefCount(newObjv[j]);
		}
                result = Tcl_EvalObjv(interp, objc, newObjv, TCL_EVAL_DIRECT);
		for (j = 0; j < objc; j++) {
		    Tcl_DecrRefCount(newObjv[j]);
		}
                ckfree((char *)newObjv);
		if (result == TCL_OK) {
                  return TCL_OK;
                }
	    }
	    /* no option at all, let the normal configure do the job */
	    infoPtr->currIdoPtr = saveIdoPtr;
	    return TCL_CONTINUE;
        }
        ioptPtr = (ItclOption *)Tcl_GetHashValue(hPtr2);
        resultPtr = ItclReportOption(interp, ioptPtr, contextIoPtr);
	infoPtr->currIdoPtr = saveIdoPtr;
	Tcl_SetObjResult(interp, resultPtr);
	return TCL_OK;
    }
    result = TCL_OK;
    /* set one or more options */
    for (i=1; i < objc; i+=2) {
	if (i+1 >= objc) {
	    Tcl_AppendResult(interp, "need option value pair", NULL);
	    result = TCL_ERROR;
	    break;
	}
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectOptions,
	        (char *) objv[i]);
        if (hPtr == NULL) {
            if (contextIclsPtr->flags & ITCL_ECLASS) {
		newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc));
                newObjv[0] = Tcl_NewStringObj("::itcl::builtin::eclassConfigure", -1);
		Tcl_IncrRefCount(newObjv[0]);
		for (j = 1; j < objc; j++) {
                    newObjv[j] = objv[j];
		    Tcl_IncrRefCount(newObjv[j]);
		}
                result = Tcl_EvalObjv(interp, objc, newObjv, TCL_EVAL_DIRECT);
		for (j = 0; j < objc; j++) {
		    Tcl_DecrRefCount(newObjv[j]);
		}
                ckfree((char *)newObjv);
		if (result == TCL_OK) {
                  continue;
                }
	    }
            hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedOptions,
	            (char *) objv[i]);
            if (hPtr != NULL) {
	        /* the option is delegated */
                idoPtr = (ItclDelegatedOption *)Tcl_GetHashValue(hPtr);
                icPtr = idoPtr->icPtr;
                val = ItclGetInstanceVar(interp,
	                Tcl_GetString(icPtr->ivPtr->namePtr),
                        NULL, contextIoPtr, icPtr->ivPtr->iclsPtr);
                if ((val != NULL) && (strlen(val) > 0)) {
	            if (idoPtr->asPtr != NULL) {
                        icPtr->ivPtr->iclsPtr->infoPtr->currIdoPtr = idoPtr;
	            }
	            newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+2));
	            newObjv[0] = Tcl_NewStringObj(val, -1);
	            Tcl_IncrRefCount(newObjv[0]);
	            newObjv[1] = Tcl_NewStringObj("configure", 9);
	            Tcl_IncrRefCount(newObjv[1]);
	            if (idoPtr->asPtr != NULL) {
	                newObjv[2] = idoPtr->asPtr;
	            } else {
	                newObjv[2] = objv[i];
	            }
	            Tcl_IncrRefCount(newObjv[2]);
	            newObjv[3] = objv[i+1];
	            objPtr = Tcl_NewStringObj(val, -1);
	            Tcl_IncrRefCount(objPtr);
	            oPtr = Tcl_GetObjectFromObj(interp, objPtr);
	            if (oPtr != NULL) {
                        ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
                                infoPtr->object_meta_type);
	                infoPtr->currContextIclsPtr = ioPtr->iclsPtr;
	            }
	            Tcl_DecrRefCount(objPtr);
                    ItclShowArgs(1, "extended eval delegated option", 4,
		            newObjv);
                    result = Tcl_EvalObjv(interp, 4, newObjv, TCL_EVAL_DIRECT);
	            Tcl_DecrRefCount(newObjv[2]);
                    Tcl_DecrRefCount(newObjv[1]);
                    Tcl_DecrRefCount(newObjv[0]);
                    ckfree((char *)newObjv);
                    icPtr->ivPtr->iclsPtr->infoPtr->currIdoPtr = NULL;
	            if (oPtr != NULL) {
	                infoPtr->currContextIclsPtr = NULL;
	            }
                    continue;
                } else {
	            Tcl_AppendResult(interp, "INTERNAL ERROR component not ",
		            "found or not set in ItclExtendedConfigure ",
			    "delegated option", NULL);
	            return TCL_ERROR;
	        }
	    }
	}
        if (hPtr == NULL) {
	    infoPtr->unparsedObjc += 2;
	    if (infoPtr->unparsedObjv == NULL) {
	        infoPtr->unparsedObjc++; /* keep the first slot for
		                            correct working !! */
	        infoPtr->unparsedObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)
	                *(infoPtr->unparsedObjc));
	        infoPtr->unparsedObjv[0] = objv[0];
	    } else {
	        infoPtr->unparsedObjv = (Tcl_Obj **)ckrealloc(
	                (char *)infoPtr->unparsedObjv, sizeof(Tcl_Obj *)
	                *(infoPtr->unparsedObjc));
	    }
	    infoPtr->unparsedObjv[infoPtr->unparsedObjc-2] = objv[i];
	    Tcl_IncrRefCount(infoPtr->unparsedObjv[infoPtr->unparsedObjc-2]);
	    infoPtr->unparsedObjv[infoPtr->unparsedObjc-1] = objv[i+1];
	    Tcl_IncrRefCount(infoPtr->unparsedObjv[infoPtr->unparsedObjc-1]);
	    /* check if normal public variable/common ? */
	    /* FIXME !!! temporary */
	    continue;
        }
        ioptPtr = (ItclOption *)Tcl_GetHashValue(hPtr);
        if (ioptPtr->flags & ITCL_OPTION_READONLY) {
	    if (infoPtr->currIoPtr == NULL) {
	        /* allow only setting during instance creation
		 * infoPtr->currIoPtr != NULL during instance creation
		 */
	        Tcl_AppendResult(interp, "option \"",
	                Tcl_GetString(ioptPtr->namePtr),
		        "\" can only be set at instance creation", NULL);
	        return TCL_ERROR;
	    }
	}
        if (ioptPtr->validateMethodPtr != NULL) {
	    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * 3);
	    newObjv[0] = ioptPtr->validateMethodPtr;
	    newObjv[1] = objv[i];
	    newObjv[2] = objv[i+1];
	    infoPtr->inOptionHandling = 1;
	    saveNsPtr = Tcl_GetCurrentNamespace(interp);
	    Itcl_SetCallFrameNamespace(interp, contextIclsPtr->nsPtr);
            ItclShowArgs(1, "EVAL validatemethod", 3, newObjv);
            result = Tcl_EvalObjv(interp, 3, newObjv, TCL_EVAL_DIRECT);
	    Itcl_SetCallFrameNamespace(interp, saveNsPtr);
	    infoPtr->inOptionHandling = 0;
            ckfree((char *)newObjv);
	    if (result != TCL_OK) {
	        break;
	    }
	}
	configureMethodPtr = NULL;
	evalNsPtr = NULL;
	if (ioptPtr->configureMethodPtr != NULL) {
	    configureMethodPtr = ioptPtr->configureMethodPtr;
	    Tcl_IncrRefCount(configureMethodPtr);
	    evalNsPtr = ioptPtr->iclsPtr->nsPtr;
	}
	if (ioptPtr->configureMethodVarPtr != NULL) {
	    val = ItclGetInstanceVar(interp,
	            Tcl_GetString(ioptPtr->configureMethodVarPtr), NULL,
		    contextIoPtr, ioptPtr->iclsPtr);
	    if (val == NULL) {
	        Tcl_AppendResult(interp, "configure cannot get value for",
		        " configuremethodvar \"",
			Tcl_GetString(ioptPtr->configureMethodVarPtr),
			"\"", NULL);
		return TCL_ERROR;
	    }
	    objPtr = Tcl_NewStringObj(val, -1);
	    hPtr = Tcl_FindHashEntry(&contextIoPtr->iclsPtr->resolveCmds,
	        (char *)objPtr);
	    Tcl_DecrRefCount(objPtr);
            if (hPtr != NULL) {
		ItclMemberFunc *imPtr;
		ItclCmdLookup *clookup;
		clookup = (ItclCmdLookup *)Tcl_GetHashValue(hPtr);
		imPtr = clookup->imPtr;
	        evalNsPtr = imPtr->iclsPtr->nsPtr;
	    } else {
		Tcl_AppendResult(interp, "cannot find method \"",
		        val, "\" found in configuremethodvar", NULL);
		return TCL_ERROR;
	    }
	    configureMethodPtr = Tcl_NewStringObj(val, -1);
	    Tcl_IncrRefCount(configureMethodPtr);
	}
        if (configureMethodPtr != NULL) {
	    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*3);
	    newObjv[0] = configureMethodPtr;
	    Tcl_IncrRefCount(newObjv[0]);
	    newObjv[1] = objv[i];
	    Tcl_IncrRefCount(newObjv[1]);
	    newObjv[2] = objv[i+1];
	    Tcl_IncrRefCount(newObjv[2]);
	    saveNsPtr = Tcl_GetCurrentNamespace(interp);
	    Itcl_SetCallFrameNamespace(interp, evalNsPtr);
            ItclShowArgs(1, "EVAL configuremethod", 3, newObjv);
            result = Tcl_EvalObjv(interp, 3, newObjv, TCL_EVAL_DIRECT);
	    Tcl_DecrRefCount(newObjv[0]);
	    Tcl_DecrRefCount(newObjv[1]);
	    Tcl_DecrRefCount(newObjv[2]);
            ckfree((char *)newObjv);
	    Itcl_SetCallFrameNamespace(interp, saveNsPtr);
	    Tcl_DecrRefCount(configureMethodPtr);
	    if (result != TCL_OK) {
	        break;
	    }
	} else {
	    if (ItclSetInstanceVar(interp, "itcl_options",
	            Tcl_GetString(objv[i]), Tcl_GetString(objv[i+1]),
		    contextIoPtr, ioptPtr->iclsPtr) == NULL) {
		result = TCL_ERROR;
	        break;
	    }
	}
	Tcl_ResetResult(interp);
        result = TCL_OK;
    }
    if (infoPtr->unparsedObjc > 0) {
	if (result == TCL_OK) {
            return TCL_CONTINUE;
        }
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclExtendedCget()
 *
 *  Invoked whenever the user issues the "cget" method for an object.
 *  If the class is NOT ITCL_CLASS
 *  Handles the following syntax:
 *
 *    <objName> cget -<option>
 *
 *  Allows access to public variables as if they were configuration
 *  options.  Mimics the behavior of the usual "cget" method for
 *  Tk widgets.  Returns the current value of the public variable
 *  with name <option>.
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ItclExtendedCget(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashEntry *hPtr2;
    Tcl_HashEntry *hPtr3;
    Tcl_Obj *objPtr2;
    Tcl_Obj *objPtr;
    Tcl_Object oPtr;
    Tcl_Obj *methodNamePtr;
    Tcl_Obj **newObjv;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclDelegatedFunction *idmPtr;
    ItclDelegatedOption *idoPtr;
    ItclComponent *icPtr;
    ItclObjectInfo *infoPtr;
    ItclOption *ioptPtr;
    ItclObject *ioPtr;
    const char *val;
    int i;
    int result;

    ItclShowArgs(1,"ItclExtendedCget", objc, objv);
    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if ((contextIoPtr == NULL) || objc != 2) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "improper usage: should be \"object cget -option\"",
            NULL);
        return TCL_ERROR;
    }

    /*
     *  BE CAREFUL:  work in the virtual scope!
     */
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    infoPtr = contextIclsPtr->infoPtr;
    if (infoPtr->currContextIclsPtr != NULL) {
        contextIclsPtr = infoPtr->currContextIclsPtr;
    }

    hPtr = NULL;
    /* first check if method cget is delegated */
    methodNamePtr = Tcl_NewStringObj("*", -1);
    hPtr = Tcl_FindHashEntry(&contextIclsPtr->delegatedFunctions, (char *)
            methodNamePtr);
    if (hPtr != NULL) {
        idmPtr = (ItclDelegatedFunction *)Tcl_GetHashValue(hPtr);
	Tcl_SetStringObj(methodNamePtr, "cget", -1);
        hPtr = Tcl_FindHashEntry(&idmPtr->exceptions, (char *)methodNamePtr);
        if (hPtr == NULL) {
	    icPtr = idmPtr->icPtr;
	    val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr),
	            NULL, contextIoPtr, contextIclsPtr);
            if (val != NULL) {
	        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+1));
	        newObjv[0] = Tcl_NewStringObj(val, -1);
	        Tcl_IncrRefCount(newObjv[0]);
	        newObjv[1] = Tcl_NewStringObj("cget", 4);
	        Tcl_IncrRefCount(newObjv[1]);
		for(i=1;i<objc;i++) {
		    newObjv[i+1] = objv[i];
		}
		objPtr = Tcl_NewStringObj(val, -1);
	        Tcl_IncrRefCount(objPtr);
	        oPtr = Tcl_GetObjectFromObj(interp, objPtr);
	        if (oPtr != NULL) {
                    ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
                            infoPtr->object_meta_type);
	            infoPtr->currContextIclsPtr = ioPtr->iclsPtr;
	        }
		ItclShowArgs(1, "DELEGATED EVAL", objc+1, newObjv);
                result = Tcl_EvalObjv(interp, objc+1, newObjv, TCL_EVAL_DIRECT);
	        Tcl_DecrRefCount(newObjv[0]);
	        Tcl_DecrRefCount(newObjv[1]);
	        Tcl_DecrRefCount(objPtr);
	        if (oPtr != NULL) {
	            infoPtr->currContextIclsPtr = NULL;
	        }
                Tcl_DecrRefCount(methodNamePtr);
                return result;
	    }
	}
    }
    Tcl_DecrRefCount(methodNamePtr);
    if (objc == 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "option");
        return TCL_ERROR;
    }
    /* now do the hard work */
    /* first handle delegated options */
    hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedOptions, (char *)
            objv[1]);
    hPtr3 = Tcl_FindHashEntry(&contextIoPtr->objectOptions, (char *)
            objv[1]);
    hPtr2 = NULL;
    if (hPtr == NULL) {
	objPtr2 = Tcl_NewStringObj("*", -1);
        /* check for "*" option delegated */
        hPtr = Tcl_FindHashEntry(&contextIoPtr->objectDelegatedOptions, (char *)
                objPtr2);
	Tcl_DecrRefCount(objPtr2);
        hPtr2 = Tcl_FindHashEntry(&contextIoPtr->objectOptions, (char *)
                objv[1]);
    }
    if ((hPtr != NULL) && (hPtr2 == NULL) && (hPtr3 == NULL)) {
	/* the option is delegated */
        idoPtr = (ItclDelegatedOption *)Tcl_GetHashValue(hPtr);
	/* if the option is in the exceptions, do nothing */
        hPtr = Tcl_FindHashEntry(&idoPtr->exceptions, (char *)
                objv[1]);
	if (hPtr) {
	    return TCL_CONTINUE;
	}
        icPtr = idoPtr->icPtr;
	if (icPtr->ivPtr->flags & ITCL_COMMON) {
            val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr),
                    NULL, contextIoPtr, icPtr->ivPtr->iclsPtr);
	} else {
            val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr),
                    NULL, contextIoPtr, icPtr->ivPtr->iclsPtr);
	}
        if ((val != NULL) && (strlen(val) > 0)) {
	    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*(objc+1));
	    newObjv[0] = Tcl_NewStringObj(val, -1);
	    Tcl_IncrRefCount(newObjv[0]);
	    newObjv[1] = Tcl_NewStringObj("cget", 4);
	    Tcl_IncrRefCount(newObjv[1]);
	    for(i=1;i<objc;i++) {
		if (strcmp(Tcl_GetString(idoPtr->namePtr),
		        Tcl_GetString(objv[i])) == 0) {
		    if (idoPtr->asPtr != NULL) {
		        newObjv[i+1] = idoPtr->asPtr;
		    } else {
	                newObjv[i+1] = objv[i];
		    }
		} else {
	            newObjv[i+1] = objv[i];
	        }
	    }
	    objPtr = Tcl_NewStringObj(val, -1);
	    Tcl_IncrRefCount(objPtr);
	    oPtr = Tcl_GetObjectFromObj(interp, objPtr);
	    if (oPtr != NULL) {
                ioPtr = (ItclObject *)Tcl_ObjectGetMetadata(oPtr,
                        infoPtr->object_meta_type);
	        infoPtr->currContextIclsPtr = ioPtr->iclsPtr;
	    }
	    ItclShowArgs(1, "ExtendedCget delegated option", objc+1, newObjv);
            result = Tcl_EvalObjv(interp, objc+1, newObjv, TCL_EVAL_DIRECT);
	    Tcl_DecrRefCount(newObjv[0]);
	    Tcl_DecrRefCount(newObjv[1]);
	    Tcl_DecrRefCount(objPtr);
	    if (oPtr != NULL) {
	        infoPtr->currContextIclsPtr = NULL;
	    }
	    ckfree((char *)newObjv);
            return result;
        } else {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "component \"",
	            Tcl_GetString(icPtr->namePtr),
	            "\" is undefined, needed for option \"",
		    Tcl_GetString(objv[1]),
	            "\"", NULL);
	    return TCL_ERROR;
	}
    }

    /* now look if it is an option at all */
    if ((hPtr2 == NULL) && (hPtr3 == NULL)) {
	/* no option at all, let the normal configure do the job */
	return TCL_CONTINUE;
    }
    if (hPtr3 != NULL) {
        ioptPtr = (ItclOption *)Tcl_GetHashValue(hPtr3);
    } else {
        ioptPtr = (ItclOption *)Tcl_GetHashValue(hPtr2);
    }
    result = TCL_CONTINUE;
    if (ioptPtr->cgetMethodPtr != NULL) {
        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*2);
        newObjv[0] = ioptPtr->cgetMethodPtr;
	Tcl_IncrRefCount(newObjv[0]);
        newObjv[1] = objv[1];
	Tcl_IncrRefCount(newObjv[1]);
	ItclShowArgs(1, "eval cget method", objc, newObjv);
        result = Tcl_EvalObjv(interp, objc, newObjv, TCL_EVAL_DIRECT);
	Tcl_DecrRefCount(newObjv[1]);
	Tcl_DecrRefCount(newObjv[0]);
        ckfree((char *)newObjv);
    } else {
        val = ItclGetInstanceVar(interp, "itcl_options",
                Tcl_GetString(ioptPtr->namePtr),
		contextIoPtr, ioptPtr->iclsPtr);
        if (val) {
            Tcl_SetObjResult(interp, Tcl_NewStringObj(val, -1));
        } else {
            Tcl_SetObjResult(interp, Tcl_NewStringObj("<undefined>", -1));
        }
        result = TCL_OK;
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  ItclExtendedSetGet()
 *
 *  Invoked whenever the user writes to a methodvariable or calls the method
 *  with the same name as the variable.
 *  only for not ITCL_CLASS classes
 *  Handles the following syntax:
 *
 *    <objName> setget varName ?<value>?
 *
 *  Allows access to methodvariables as if they hat a setter and getter
 *  method
 *  With no arguments, this command returns the current
 *  value of the variable.  If <value> is specified,
 *  this sets the variable to the value calling a callback if exists:
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
ItclExtendedSetGet(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    Tcl_HashEntry *hPtr;
    Tcl_Obj **newObjv;
    ItclMethodVariable *imvPtr;
    ItclObjectInfo *infoPtr;
    const char *usageStr;
    const char *val;
    int result;
    int setValue;

    ItclShowArgs(1, "ItclExtendedSetGet", objc, objv);
    imvPtr = NULL;
    result = TCL_OK;
    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    usageStr = "improper usage: should be \"object setget varName ?value?\"";
    if (contextIoPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                usageStr, NULL);
        return TCL_ERROR;
    }

    /*
     *  BE CAREFUL:  work in the virtual scope!
     */
    if (contextIoPtr != NULL) {
        contextIclsPtr = contextIoPtr->iclsPtr;
    }
    infoPtr = contextIclsPtr->infoPtr;
    if (infoPtr->currContextIclsPtr != NULL) {
        contextIclsPtr = infoPtr->currContextIclsPtr;
    }

    hPtr = NULL;
    if (objc < 2) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
                usageStr, NULL);
        return TCL_ERROR;
    }
    /* look if it is an methodvariable at all */
    hPtr = Tcl_FindHashEntry(&contextIoPtr->objectMethodVariables,
            (char *) objv[1]);
    if (hPtr == NULL) {
	Tcl_AppendResult(interp, "no such methodvariable \"",
	        Tcl_GetString(objv[1]), "\"", NULL);
	return TCL_ERROR;
    }
    imvPtr = (ItclMethodVariable *)Tcl_GetHashValue(hPtr);
    if (objc == 2) {
        val = ItclGetInstanceVar(interp, Tcl_GetString(objv[1]), NULL,
	        contextIoPtr, imvPtr->iclsPtr);
        if (val == NULL) {
            result = TCL_ERROR;
        } else {
	   Tcl_SetObjResult(interp, Tcl_NewStringObj(val, -1));
	}
        return result;
    }
    imvPtr = (ItclMethodVariable *)Tcl_GetHashValue(hPtr);
    result = TCL_OK;
    setValue = 1;
    if (imvPtr->callbackPtr != NULL) {
        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *)*3);
        newObjv[0] = imvPtr->callbackPtr;
        Tcl_IncrRefCount(newObjv[0]);
        newObjv[1] = objv[1];
        Tcl_IncrRefCount(newObjv[1]);
        newObjv[2] = objv[2];
        Tcl_IncrRefCount(newObjv[2]);
        result = Tcl_EvalObjv(interp, 3, newObjv, TCL_EVAL_DIRECT);
        Tcl_DecrRefCount(newObjv[0]);
        Tcl_DecrRefCount(newObjv[1]);
        Tcl_DecrRefCount(newObjv[2]);
        ckfree((char *)newObjv);
    }
    if (result == TCL_OK) {
        Tcl_GetIntFromObj(interp, Tcl_GetObjResult(interp), &setValue);
	/* if setValue != 0 set the new value of the variable here */
	if (setValue) {
            if (ItclSetInstanceVar(interp, Tcl_GetString(objv[1]), NULL,
	            Tcl_GetString(objv[2]), contextIoPtr,
		    imvPtr->iclsPtr) == NULL) {
                result = TCL_ERROR;
            }
        }
    }
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInstallComponentCmd()
 *
 *  Invoked whenever the user issues the "installcomponent" method for an
 *  object.
 *  Handles the following syntax:
 *
 *    installcomponent <componentName> using <widgetClassName> <widgetPathName>
 *      ?-option value -option value ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiInstallComponentCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    FOREACH_HASH_DECLS;
    Tcl_Obj ** newObjv;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclDelegatedOption *idoPtr;
    const char *usageStr;
    const char *componentName;
    const char *componentValue;
    const char *token;
    int numOpts;
    int result;


    ItclShowArgs(1, "Itcl_BiInstallComponentCmd", objc, objv);
    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (contextIoPtr == NULL) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "improper usage: should be \"object installcomponent \"",
            NULL);
        return TCL_ERROR;
    }
    if (objc < 5) {
	/* FIXME strip off the :: parts here properly*/
        token = Tcl_GetString(objv[0])+2;
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"", token, " <componentName> using",
	    " <widgetClassName> <widgetPathName>",
	    " ?-option value -option value ...?\"",
            NULL);
        return TCL_ERROR;
    }

    /* get component name and check, if it exists */
    token = Tcl_GetString(objv[1]);
    if (contextIclsPtr == NULL) {
        Tcl_AppendResult(interp, "cannot find context class for object \"",
	        Tcl_GetCommandName(interp, contextIoPtr->accessCmd), "\"",
		NULL);
        return TCL_ERROR;
    }
    if (!(contextIclsPtr->flags & (ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
        Tcl_AppendResult(interp, "no such method \"installcomponent\"", NULL);
	return TCL_ERROR;
    }
    hPtr = Tcl_FindHashEntry(&contextIclsPtr->components, (char *)objv[1]);
    if (hPtr == NULL) {
	numOpts = 0;
	FOREACH_HASH_VALUE(idoPtr, &contextIoPtr->objectDelegatedOptions) {
            if (idoPtr == NULL) {
                /* FIXME need code here !! */
	    }
	    numOpts++;
	}
	if (numOpts == 0) {
	    /* there are no delegated options, so no problem that the
	     * component does not exist. We have nothing to do */
	    return TCL_OK;
	}
	Tcl_AppendResult(interp, "class \"",
	        Tcl_GetString(contextIclsPtr->namePtr),
	        "\" has no component \"",
		Tcl_GetString(objv[1]), "\"", NULL);
        return TCL_ERROR;
    }
    if (contextIclsPtr->flags & ITCL_TYPE) {
        Tcl_Obj *objPtr;
        usageStr = "usage: installcomponent <componentName> using <widgetType> <widgetPath> ?-option value ...?";
        if (objc < 4) {
            Tcl_AppendResult(interp, usageStr, NULL);
	    return TCL_ERROR;
        }
        if (strcmp(Tcl_GetString(objv[2]), "using") != 0) {
            Tcl_AppendResult(interp, usageStr, NULL);
	    return TCL_ERROR;
        }
        componentName = Tcl_GetString(objv[1]);
        /* as it is no widget, we don't need to check for delegated option */
        newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc - 3));
        memcpy(newObjv, objv + 3, sizeof(Tcl_Obj *) * ((objc - 3)));
        ItclShowArgs(1, "BiInstallComponent", objc - 3, newObjv);
        result = Tcl_EvalObjv(interp, objc - 3, newObjv, 0);
	ckfree((char *)newObjv);
        if (result != TCL_OK) {
            return result;
        }
        componentValue = Tcl_GetString(Tcl_GetObjResult(interp));
        objPtr = Tcl_NewStringObj(ITCL_VARIABLES_NAMESPACE, -1);
	Tcl_AppendToObj(objPtr,
		(Tcl_GetObjectNamespace(contextIclsPtr->oPtr))->fullName, -1);
        Tcl_AppendToObj(objPtr, "::", -1);
        Tcl_AppendToObj(objPtr, componentName, -1);

        Tcl_SetVar2(interp, Tcl_GetString(objPtr), NULL, componentValue, 0);
	Tcl_DecrRefCount(objPtr);

    } else {
	newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc + 1));
	newObjv[0] = Tcl_NewStringObj("::itcl::builtin::installcomponent", -1);
	Tcl_IncrRefCount(newObjv[0]);
        memcpy(newObjv, objv + 1, sizeof(Tcl_Obj *) * ((objc - 1)));
        result = Tcl_EvalObjv(interp, objc, newObjv, 0);
	Tcl_DecrRefCount(newObjv[0]);
	ckfree((char *)newObjv);
	return result;
    }
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiDestroyCmd()
 *
 *  Invoked whenever the user issues the "destroy" method for an
 *  object.
 *  Handles the following syntax:
 *
 *    destroy
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
static int
Itcl_BiDestroyCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj **newObjv;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    int result;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiDestroyCmd", objc, objv);
    contextIoPtr = NULL;
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (contextIclsPtr == NULL) {
        Tcl_AppendResult(interp, "cannot find context class for object \"",
	        Tcl_GetCommandName(interp, contextIoPtr->accessCmd), "\"",
		NULL);
        return TCL_ERROR;
    }
    if ((objc > 1) || !(contextIclsPtr->flags &
            (ITCL_ECLASS|ITCL_TYPE|ITCL_WIDGET|ITCL_WIDGETADAPTOR))) {
	/* try to execute destroy in uplevel namespace */
	newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj *) * (objc + 2));
	newObjv[0] = Tcl_NewStringObj("uplevel", -1);
	Tcl_IncrRefCount(newObjv[0]);
	newObjv[1] = Tcl_NewStringObj("#0", -1);
	Tcl_IncrRefCount(newObjv[1]);
	newObjv[2] = Tcl_NewStringObj("destroy", -1);
	Tcl_IncrRefCount(newObjv[2]);
	memcpy(newObjv + 3, objv + 1, sizeof(Tcl_Obj *) * (objc - 1));
        ItclShowArgs(1, "DESTROY", objc + 2, newObjv);
        result = Tcl_EvalObjv(interp, objc + 2, newObjv, 0);
	Tcl_DecrRefCount(newObjv[2]);
	Tcl_DecrRefCount(newObjv[1]);
	Tcl_DecrRefCount(newObjv[0]);
	return result;
    }
    if (objc != 1) {
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"", Tcl_GetString(objv[0]), NULL);
        return TCL_ERROR;
    }

    if (contextIoPtr != NULL) {
        Tcl_Obj *objPtr = Tcl_NewObj();
        Tcl_GetCommandFullName(interp, contextIoPtr->accessCmd, objPtr);
        Itcl_RenameCommand(interp, Tcl_GetString(objPtr), "");
	Tcl_DecrRefCount(objPtr);
        result = TCL_OK;
    } else {
        result = Itcl_DeleteClass(interp, contextIclsPtr);
    }
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiCallInstanceCmd()
 *
 *  Invoked whenever the a script generated by mytypemethod, mymethod or
 *  myproc is evauated later on:
 *  Handles the following syntax:
 *
 *    callinstance <instanceName> ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiCallInstanceCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj **newObjv;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObject *ioPtr;
    const char *token;
    int result;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiCallInstanceCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc < 2) {
        token = Tcl_GetString(objv[0]);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"", token, " <instanceName>",
            NULL);
        return TCL_ERROR;
    }

    hPtr = Tcl_FindHashEntry(&contextIclsPtr->infoPtr->instances,
            Tcl_GetString(objv[1]));
    if (hPtr == NULL) {
	Tcl_AppendResult(interp,
	        "no such instanceName \"",
		Tcl_GetString(objv[1]), "\"", NULL);
        return TCL_ERROR;
    }
    ioPtr = (ItclObject *)Tcl_GetHashValue(hPtr);
    objPtr =Tcl_NewObj();
    Tcl_GetCommandFullName(interp, ioPtr->accessCmd, objPtr);
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*) * (objc - 1));
    newObjv[0] = objPtr;
    Tcl_IncrRefCount(newObjv[0]);
    memcpy(newObjv + 1, objv + 2, sizeof(Tcl_Obj *) * (objc - 2));
    result = Tcl_EvalObjv(interp, objc - 1, newObjv, 0);
    Tcl_DecrRefCount(newObjv[0]);
    ckfree((char *)newObjv);
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiGetInstanceVarCmd()
 *
 *  Invoked whenever the a script generated by mytypevar, myvar or
 *  mycommon is evauated later on:
 *  Handles the following syntax:
 *
 *    getinstancevar <instanceName> ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiGetInstanceVarCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_Obj *objPtr;
    Tcl_Obj **newObjv;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    ItclObject *ioPtr;
    const char *token;
    int result;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiGetInstanceVarCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }

    if (objc < 2) {
        token = Tcl_GetString(objv[0]);
        Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
            "wrong # args: should be \"", token, " <instanceName>",
            NULL);
        return TCL_ERROR;
    }

    hPtr = Tcl_FindHashEntry(&contextIclsPtr->infoPtr->instances,
            Tcl_GetString(objv[1]));
    if (hPtr == NULL) {
	Tcl_AppendResult(interp,
	        "no such instanceName \"",
		Tcl_GetString(objv[1]), "\"", NULL);
        return TCL_ERROR;
    }
    ioPtr = (ItclObject *)Tcl_GetHashValue(hPtr);
    objPtr = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, ioPtr->accessCmd, objPtr);
    newObjv = (Tcl_Obj **)ckalloc(sizeof(Tcl_Obj*) * (objc - 1));
    newObjv[0] = objPtr;
    Tcl_IncrRefCount(newObjv[0]);
    memcpy(newObjv + 1, objv + 2, sizeof(Tcl_Obj *) * (objc - 2));
    result = Tcl_EvalObjv(interp, objc - 1, newObjv, 0);
    Tcl_DecrRefCount(newObjv[0]);
    return result;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiMyTypeMethodCmd()
 *
 *  Invoked when a user calls mytypemethod
 *
 *  Handles the following syntax:
 *
 *    mytypemethod ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiMyTypeMethodCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *objPtr;
    Tcl_Obj *resultPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    int i;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiMyTypeMethodCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc < 2) {
        Tcl_AppendResult(interp, "usage: mytypemethod <name>", NULL);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(contextIclsPtr->nsPtr->fullName, -1);
    resultPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, resultPtr, objPtr);

    for (i = 1; i < objc; i++) {
	Tcl_ListObjAppendElement(interp, resultPtr, objv[i]);
    }
    Tcl_SetObjResult(interp, resultPtr);

    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiMyMethodCmd()
 *
 *  Invoked when a user calls mymethod
 *
 *  Handles the following syntax:
 *
 *    mymethod ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiMyMethodCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *resultPtr;
    int i;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiMyMethodCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
	resultPtr = Tcl_NewListObj(0, NULL);
	Tcl_ListObjAppendElement(interp, resultPtr,
	        Tcl_NewStringObj("::itcl::builtin::callinstance", -1));
	Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(
		(Tcl_GetObjectNamespace(contextIoPtr->oPtr))->fullName, -1));
	for (i = 1; i < objc; i++) {
	    Tcl_ListObjAppendElement(interp, resultPtr, objv[i]);
	}
	Tcl_SetObjResult(interp, resultPtr);
        return TCL_OK;
    }

    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiMyProcCmd()
 *
 *  Invoked when a user calls myproc
 *
 *  Handles the following syntax:
 *
 *    myproc ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiMyProcCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *objPtr;
    Tcl_Obj *resultPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    int i;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiMyProcCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc < 2) {
        Tcl_AppendResult(interp, "usage: myproc <name>", NULL);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(contextIclsPtr->nsPtr->fullName, -1);
    Tcl_AppendToObj(objPtr, "::", -1);
    Tcl_AppendToObj(objPtr, Tcl_GetString(objv[1]), -1);
    resultPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, resultPtr, objPtr);

    for (i = 2; i < objc; i++) {
	Tcl_ListObjAppendElement(interp, resultPtr, objv[i]);
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiMyTypeVarCmd()
 *
 *  Invoked when a user calls mytypevar
 *
 *  Handles the following syntax:
 *
 *    mytypevar ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiMyTypeVarCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *objPtr;
    Tcl_Obj *resultPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    int i;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiMyTypeVarCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc < 2) {
        Tcl_AppendResult(interp, "usage: mytypevar <name>", NULL);
        return TCL_ERROR;
    }
    objPtr = Tcl_NewStringObj(contextIclsPtr->nsPtr->fullName, -1);
    Tcl_AppendToObj(objPtr, "::", -1);
    Tcl_AppendToObj(objPtr, Tcl_GetString(objv[1]), -1);
    resultPtr = Tcl_NewListObj(0, NULL);
    Tcl_ListObjAppendElement(interp, resultPtr, objPtr);

    for (i = 2; i < objc; i++) {
	Tcl_ListObjAppendElement(interp, resultPtr, objv[i]);
    }
    Tcl_SetObjResult(interp, resultPtr);

    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiMyVarCmd()
 *
 *  Invoked when a user calls myvar
 *
 *  Handles the following syntax:
 *
 *    myvar ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiMyVarCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_Obj *resultPtr;
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiMyVarCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        resultPtr = Tcl_NewStringObj(Tcl_GetString(contextIoPtr->varNsNamePtr),
	        -1);
	Tcl_AppendToObj(resultPtr, "::", -1);
	Tcl_AppendToObj(resultPtr, Tcl_GetString(contextIclsPtr->namePtr), -1);
	Tcl_AppendToObj(resultPtr, "::", -1);
	Tcl_AppendToObj(resultPtr, Tcl_GetString(objv[1]), -1);
	Tcl_SetObjResult(interp, resultPtr);
    }
    return TCL_OK;
}
/*
 * ------------------------------------------------------------------------
 *  Itcl_BiItclHullCmd()
 *
 *  Invoked when a user calls itcl_hull
 *
 *  Handles the following syntax:
 *
 *    itcl_hull ?arg arg ...?
 *
 * ------------------------------------------------------------------------
 */
/* ARGSUSED */
int
Itcl_BiItclHullCmd(
    void *clientData,   /* class definition */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    ItclClass *contextIclsPtr;
    ItclObject *contextIoPtr;
    const char *val;

    /*
     *  Make sure that this command is being invoked in the proper
     *  context.
     */
    ItclShowArgs(1, "Itcl_BiItclHullCmd", objc, objv);
    contextIclsPtr = NULL;
    if (Itcl_GetContext(interp, &contextIclsPtr, &contextIoPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (contextIoPtr != NULL) {
        val = ItclGetInstanceVar(interp, "itcl_hull", NULL,
	        contextIoPtr, contextIclsPtr);
	Tcl_SetObjResult(interp, Tcl_NewStringObj(val, -1));
    }
    return TCL_OK;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiCreateHullCmd()
 *
 *  Invoked by Tcl normally during evaluating constructor
 *  the "createhull" command is invoked to install and setup an
 *  ::itcl::extendedclass itcl_hull
 *  for an object.  Handles the following syntax:
 *
 *      createhull <widget_type> <widget_path> ?-class <widgetClassName>?
 *          ?<optionName> <optionValue> <optionName> <optionValue> ...?
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_BiCreateHullCmd(
    void *clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int result;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;

    ItclShowArgs(1, "Itcl_BiCreateHullCmd", objc, objv);
    if (!infoPtr->itclHullCmdsInitted) {
        result =  Tcl_EvalEx(interp, initHullCmdsScript, -1, 0);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclHullCmdsInitted = 1;
    }
    return Tcl_EvalObjv(interp, objc, objv, 0);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiSetupComponentCmd()
 *
 *  Invoked by Tcl during evaluating constructor whenever
 *  the "setupcomponent" command is invoked to install and setup an
 *  ::itcl::extendedclass component
 *  for an object.  Handles the following syntax:
 *
 *      setupcomponent <componentName> using <widgetType> <widget_path>
 *          ?<optionName> <optionValue> <optionName> <optionValue> ...?
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_BiSetupComponentCmd(
    void *clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int result;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;

    ItclShowArgs(1, "Itcl_BiSetupComponentCmd", objc, objv);
    if (!infoPtr->itclHullCmdsInitted) {
        result =  Tcl_EvalEx(interp, initHullCmdsScript, -1, 0);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclHullCmdsInitted = 1;
    }
    return Tcl_EvalObjv(interp, objc, objv, 0);
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiInitOptionsCmd()
 *
 *  Invoked by Tcl during evaluating constructor whenever
 *  the "itcl_initoptions" command is invoked to install and setup an
 *  ::itcl::extendedclass options
 *  for an object.  Handles the following syntax:
 *
 *      itcl_initoptions
 *          ?<optionName> <optionValue> <optionName> <optionValue> ...?
 * FIXME !!!! seems no longer been used !!!
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_BiInitOptionsCmd(
    void *clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int result;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;
    ItclClass *iclsPtr;
    ItclObject *ioPtr;
    ItclDelegatedOption *idoptPtr;
    ItclOption *ioptPtr;
    FOREACH_HASH_DECLS;

    /* instead ::itcl::builtin::initoptions in ../library/itclHullCmds.tcl is used !! */
    ItclShowArgs(1, "Itcl_BiInitOptionsCmd", objc, objv);
    if (!infoPtr->itclHullCmdsInitted) {
        result =  Tcl_EvalEx(interp, initHullCmdsScript, -1, 0);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclHullCmdsInitted = 1;
    }
    result = Tcl_EvalObjv(interp, objc, objv, 0);
    iclsPtr = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    /* first handle delegated options */
    FOREACH_HASH_VALUE(idoptPtr, &ioPtr->objectDelegatedOptions) {
fprintf(stderr, "delopt!%s!\n", Tcl_GetString(idoptPtr->namePtr));
    }
    FOREACH_HASH_VALUE(ioptPtr, &ioPtr->objectOptions) {
fprintf(stderr, "opt!%s!\n", Tcl_GetString(ioptPtr->namePtr));
    }
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiKeepComponentOptionCmd()
 *
 *  Invoked by Tcl during evaluating constructor whenever
 *  the "keepcomponentoption" command is invoked to list the options
 *  to be kept when and ::itcl::extendedclass component has been setup
 *  for an object.  Handles the following syntax:
 *
 *      keepcomponentoption <componentName> <optionName> ?<optionName> ...?
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_BiKeepComponentOptionCmd(
    void *clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    int result;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;

    ItclShowArgs(1, "Itcl_BiKeepComponentOptionCmd", objc, objv);
    if (!infoPtr->itclHullCmdsInitted) {
        result =  Tcl_EvalEx(interp, initHullCmdsScript, -1, 0);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclHullCmdsInitted = 1;
    }
    result =  Tcl_EvalObjv(interp, objc, objv, 0);
    return result;
}

/*
 * ------------------------------------------------------------------------
 *  Itcl_BiIgnoreComponentOptionCmd()
 *
 *  Invoked by Tcl during evaluating constructor whenever
 *  the "keepcomponentoption" command is invoked to list the options
 *  to be kept when and ::itcl::extendedclass component has been setup
 *  for an object.  Handles the following syntax:
 *
 *      ignorecomponentoption <componentName> <optionName> ?<optionName> ...?
 *
 * ------------------------------------------------------------------------
 */
static int
Itcl_BiIgnoreComponentOptionCmd(
    void *clientData,   /* info for all known objects */
    Tcl_Interp *interp,      /* current interpreter */
    int objc,                /* number of arguments */
    Tcl_Obj *const objv[])   /* argument objects */
{
    Tcl_HashEntry *hPtr;
    Tcl_HashEntry *hPtr2;
    Tcl_Obj *objPtr;
    ItclClass *iclsPtr;
    ItclObject *ioPtr;
    ItclDelegatedOption *idoPtr;
    ItclComponent *icPtr;
    const char *val;
    int idx;
    int isNew;
    int result;
    ItclObjectInfo *infoPtr = (ItclObjectInfo*)clientData;

    ItclShowArgs(0, "Itcl_BiIgnoreComponentOptionCmd", objc, objv);
    if (!infoPtr->itclHullCmdsInitted) {
        result =  Tcl_Eval(interp, initHullCmdsScript);
        if (result != TCL_OK) {
            return result;
        }
        infoPtr->itclHullCmdsInitted = 1;
    }
    iclsPtr = NULL;
    if (Itcl_GetContext(interp, &iclsPtr, &ioPtr) != TCL_OK) {
        return TCL_ERROR;
    }
    if (objc < 3) {
	Tcl_AppendResult(interp, "wrong # args, should be: ",
	        "ignorecomponentoption component option ?option ...?", NULL);
        return TCL_ERROR;
    }
    if (ioPtr != NULL) {
        hPtr = Tcl_FindHashEntry(&ioPtr->objectComponents, (char *)objv[1]);
        if (hPtr == NULL) {
	    Tcl_AppendResult(interp,
	            "ignorecomponentoption cannot find component \"",
	            Tcl_GetString(objv[1]), "\"", NULL);
	    return TCL_ERROR;
	}
        icPtr = (ItclComponent *)Tcl_GetHashValue(hPtr);
	icPtr->haveKeptOptions = 1;
	for (idx = 2; idx < objc; idx++) {
	    hPtr = Tcl_CreateHashEntry(&icPtr->keptOptions, (char *)objv[idx],
	            &isNew);
            if (isNew) {
	        Tcl_SetHashValue(hPtr, objv[idx]);
	    }
	    hPtr2 = Tcl_CreateHashEntry(&ioPtr->objectDelegatedOptions,
	            (char *)objv[idx], &isNew);
	    if (isNew) {
		idoPtr = (ItclDelegatedOption *)ckalloc(sizeof(
		        ItclDelegatedOption));
		memset(idoPtr, 0, sizeof(ItclDelegatedOption));
		Tcl_InitObjHashTable(&idoPtr->exceptions);
		idoPtr->namePtr = objv[idx];
		Tcl_IncrRefCount(idoPtr->namePtr);
		idoPtr->resourceNamePtr = NULL;
		if (idoPtr->resourceNamePtr != NULL) {
		    Tcl_IncrRefCount(idoPtr->resourceNamePtr);
		}
		idoPtr->classNamePtr = NULL;
		if (idoPtr->classNamePtr != NULL) {
		    Tcl_IncrRefCount(idoPtr->classNamePtr);
		}
		idoPtr->icPtr = icPtr;
		idoPtr->ioptPtr = NULL;
		Tcl_SetHashValue(hPtr2, idoPtr);
                val = ItclGetInstanceVar(interp, Tcl_GetString(icPtr->namePtr),
		        NULL, ioPtr, iclsPtr);
		if (val != NULL) {
                    objPtr = Tcl_NewStringObj(val, -1);
                    Tcl_AppendToObj(objPtr, " cget ", -1);
                    Tcl_AppendToObj(objPtr, Tcl_GetString(objv[idx]), -1);
                    Tcl_IncrRefCount(objPtr);
                    result = Tcl_EvalObjEx(interp, objPtr, 0);
                    Tcl_DecrRefCount(objPtr);
		    if (result == TCL_OK) {
		        ItclSetInstanceVar(interp, "itcl_options",
		                Tcl_GetString(objv[idx]),
		                Tcl_GetString(Tcl_GetObjResult(interp)), ioPtr, iclsPtr);
		    }
                }
            }
        }
        ItclAddClassComponentDictInfo(interp, iclsPtr, icPtr);
    }
    return TCL_OK;
}
