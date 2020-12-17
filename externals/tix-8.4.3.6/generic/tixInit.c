/*	$Id: tixInit.c,v 1.20 2008/02/28 22:25:56 hobbs Exp $ */

/*
 * tixInit.c --
 *
 *	Initialze the Tix native code as well as the script library.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 * Copyright 2004 ActiveState
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixInit.c,v 1.20 2008/02/28 22:25:56 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>

/*
 * Minimum required version of Tcl and Tk. These are used when we access
 * Tcl/Tk using the stubs API.
 */

#define MIN_TCL_VERSION "8.4"
#define MIN_TK_VERSION  "8.4"

/*
 * All the Tix commands implemented in C code.
 */

static Tix_TclCmd commands[] = {
    /*
     * Commands that are part of the intrinsics:
     */
    {"tixCallMethod",           Tix_CallMethodCmd},
    {"tixChainMethod",          Tix_ChainMethodCmd},
    {"tixClass",                Tix_ClassCmd},
    {"tixDisplayStyle",         Tix_ItemStyleCmd},
    {"tixDoWhenIdle",           Tix_DoWhenIdleCmd},
    {"tixDoWhenMapped",         Tix_DoWhenMappedCmd},
    {"tixFlushX",           	Tix_FlushXCmd},
    {"tixForm",                 Tix_FormCmd},
    {"tixGeometryRequest",      Tix_GeometryRequestCmd},
    {"tixGet3DBorder",		Tix_Get3DBorderCmd},
    {"tixGetDefault",           Tix_GetDefaultCmd},
    {"tixGetMethod",            Tix_GetMethodCmd},
    {"tixGrid",     		Tix_GridCmd},
    {"tixHandleOptions",        Tix_HandleOptionsCmd},
    {"tixHList",                Tix_HListCmd},
#if !defined(__WIN32__) && !defined(MAC_OSX_TK)
    {"tixInputOnly",		Tix_InputOnlyCmd},
#endif
    {"tixManageGeometry",       Tix_ManageGeometryCmd},
    {"tixMapWindow",            Tix_MapWindowCmd},
    {"tixMoveResizeWindow",     Tix_MoveResizeWindowCmd},
#if !defined(__WIN32__) && !defined(MAC_OSX_TK)
    {"tixMwm",     		Tix_MwmCmd},
#endif
    {"tixNoteBookFrame",        Tix_NoteBookFrameCmd},
    {"tixTList",     		Tix_TListCmd},
    {"tixTmpLine",              Tix_TmpLineCmd},
    {"tixUnmapWindow",          Tix_UnmapWindowCmd},
    {"tixWidgetClass",          Tix_ClassCmd},
    {"tixWidgetDoWhenIdle",     Tix_DoWhenIdleCmd},

    {(char *) NULL,		(Tix_CmdProc)NULL}
};

typedef struct {
    char      * binding;
    int		isDebug;
    char      * fontSet;
    char      * scheme;
    char      * schemePriority;
} OptionStruct;

static OptionStruct tixOption;

/*
 * TIX_DEF_FONTSET and TIX_DEF_SCHEME should have been defined in the
 * Makefile by the configure script. We define them here just in case
 * the configure script failed to determine the proper values.
 */

#ifndef TIX_DEF_FONTSET
#define TIX_DEF_FONTSET "WmDefault"
#endif

#ifndef TIX_DEF_SCHEME
#define TIX_DEF_SCHEME "WmDefault"
#endif

#define DEF_TIX_TOOLKIT_OPTION_BINDING		"TK"
#define DEF_TIX_TOOLKIT_OPTION_DEBUG		"0"
#define DEF_TIX_TOOLKIT_OPTION_FONTSET		TIX_DEF_FONTSET
#define DEF_TIX_TOOLKIT_OPTION_SCHEME		TIX_DEF_SCHEME
#define DEF_TIX_TOOLKIT_OPTION_SCHEME_PRIORITY	"20" /* widgetDefault */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_STRING, "-binding", "binding", "TixBinding",
       DEF_TIX_TOOLKIT_OPTION_BINDING, Tk_Offset(OptionStruct, binding),
       0},
    {TK_CONFIG_BOOLEAN, "-debug", "tixDebug", "TixDebug",
       DEF_TIX_TOOLKIT_OPTION_DEBUG, Tk_Offset(OptionStruct, isDebug),
       0},
    {TK_CONFIG_STRING, "-fontset", "tixFontSet", "TixFontSet",
       DEF_TIX_TOOLKIT_OPTION_FONTSET, Tk_Offset(OptionStruct, fontSet),
       0},
    {TK_CONFIG_STRING, "-scheme", "tixScheme", "TixScheme",
       DEF_TIX_TOOLKIT_OPTION_SCHEME, Tk_Offset(OptionStruct, scheme),
       0},
    {TK_CONFIG_STRING, "-schemepriority", "tixSchemePriority", "TixSchemePriority",
       DEF_TIX_TOOLKIT_OPTION_SCHEME_PRIORITY,
       Tk_Offset(OptionStruct, schemePriority),
       0},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
       (char *) NULL, 0, 0}
};

/*----------------------------------------------------------------------
 *
 * 			Some global variables
 *
 *----------------------------------------------------------------------
 */
Tk_Uid tixNormalUid   = (Tk_Uid)NULL;
Tk_Uid tixCellUid     = (Tk_Uid)NULL;
Tk_Uid tixRowUid      = (Tk_Uid)NULL;
Tk_Uid tixColumnUid   = (Tk_Uid)NULL;
Tk_Uid tixDisabledUid = (Tk_Uid)NULL;

static int		ParseToolkitOptions _ANSI_ARGS_((Tcl_Interp * interp));

/*
 *----------------------------------------------------------------------
 * ParseToolkitOptions() --
 *
 *	Before the Tix initialized, we need to determine the toolkit
 *	options which are set by the options database.
 *
 * Results:
 *	A standard Tcl completion code (TCL_OK or TCL_ERROR).  Also
 *	leaves information in the interp's result.
 *
 * Side effects:
 *      Sets several variables in the global Tcl array "tixPriv".
 *
 *----------------------------------------------------------------------
 */

static int
ParseToolkitOptions(interp)
    Tcl_Interp * interp;
{
    char buff[10];
    int flag;

    tixOption.binding = NULL;
    tixOption.isDebug = 0;
    tixOption.fontSet = NULL;
    tixOption.scheme = NULL;
    tixOption.schemePriority = NULL;

    /*
     * The toolkit options may be set in the resources of the main
     * window, probably by using your .Xdefaults file.
     */

    if (Tk_ConfigureWidget(interp, Tk_MainWindow(interp), configSpecs,
	    0, 0, (char *) &tixOption, 0) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Now lets set the Tix toolkit variables so that the Toolkit can
     * initialize according to user options.
     */

    flag = TCL_GLOBAL_ONLY;

    Tcl_SetVar2(interp, "tix_priv", "-binding",
	tixOption.binding,    		flag);
    sprintf(buff, "%d", tixOption.isDebug);
    Tcl_SetVar2(interp, "tix_priv", "-debug", 
	buff,    		flag);
    Tcl_SetVar2(interp, "tix_priv", "-fontset", 
	tixOption.fontSet,    		flag);
    Tcl_SetVar2(interp, "tix_priv", "-scheme",  
	tixOption.scheme,     		flag);
    Tcl_SetVar2(interp, "tix_priv", "-schemepriority",
	tixOption.schemePriority,     flag);

    Tk_FreeOptions(configSpecs, (char *)&tixOption,
	Tk_Display(Tk_MainWindow(interp)), 0);

    return TCL_OK;
}


/*
 *------------------------------------------------------------------------
 * Package initialization.
 * tcl_findLibrary basename version patch initScript enVarName varName
 * We use patchLevel instead of version as we use the full patchlevel
 * in the directory naming on install.
 *------------------------------------------------------------------------
 */

static char initScript[] = "if {[info proc tixInit]==\"\"} {\n\
  proc tixInit {} {\n\
    global tix_library tix_version tix_patchLevel\n\
    rename tixInit {}\n\
    tcl_findLibrary Tix $tix_patchLevel $tix_patchLevel Init.tcl TIX_LIBRARY tix_library\n\
  }\n\
}\n\
tixInit";


/*
 *----------------------------------------------------------------------
 *
 * Tix_Init() --
 *
 *	Initialize the Tix library.
 *
 * Results:
 *	A standard Tcl completion code (TCL_OK or TCL_ERROR).  Also
 *	leaves information in the interp's result.
 *
 * Side effects:
 *	Sets "tix_library" Tcl variable, runs "Init.tcl" script from
 *      the Tix script library directory.
 * 
 *----------------------------------------------------------------------
 */

int
Tix_Init(interp)
	 Tcl_Interp * interp;
{
    Tk_Window topLevel;

    /*
     * This procedure may be called  several times for several
     * interpreters. Since some global variables are shared by
     * all of the interpreters, we initialize these variables only
     * once. The variable "globalInitialized" keeps track of this
     */
    static int globalInitialized = 0;

#ifdef USE_TCL_STUBS
    if ((Tcl_InitStubs(interp, MIN_TK_VERSION, 0) == NULL)
	    || (Tk_InitStubs(interp, MIN_TK_VERSION, 0) == NULL)) {
        return TCL_ERROR;
    }
#else
    if ((Tcl_PkgRequire(interp, "Tcl", TCL_VERSION, 1) == NULL)
	    || (Tcl_PkgRequire(interp, "Tk", TK_VERSION, 1) == NULL)) {
	return TCL_ERROR;
    }
#endif /* USE_TCL_STUBS */

    if (Tcl_PkgProvide(interp, "Tix", TIX_PATCH_LEVEL) != TCL_OK) {
        return TCL_ERROR;
    }

    if (!globalInitialized) {
	globalInitialized = 1;

	/*
	 * Initialize the global variables shared by all interpreters
	 */
	tixNormalUid   = Tk_GetUid("normal");
	tixCellUid     = Tk_GetUid("cell");
	tixRowUid      = Tk_GetUid("row");
	tixColumnUid   = Tk_GetUid("column");
	tixDisabledUid = Tk_GetUid("disabled");

#if !defined(__WIN32__) && !defined(MAC_OSX_TK)
	/* This is for tixMwm command */
	Tk_CreateGenericHandler(TixMwmProtocolHandler, NULL);
#endif

	/*
         * Initialize the image readers
         */

	Tk_CreateImageType(&tixPixmapImageType);
	Tk_CreateImageType(&tixCompoundImageType);

	/*
         * Initialize the display item subsystem.
         */

        TixInitializeDisplayItems();
    }

    /*
     * Initialize the per-interpreter variables
     */

    /*  Set the "tix_version" variable */
    Tcl_SetVar(interp, "tix_version",    TIX_VERSION,    TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tix_patchLevel", TIX_PATCH_LEVEL,TCL_GLOBAL_ONLY);
    Tcl_SetVar(interp, "tix_release",    TIX_RELEASE,    TCL_GLOBAL_ONLY);

    /*
     * Initialize the Tix commands
     */

    topLevel = Tk_MainWindow(interp);
    Tix_CreateCommands(interp, commands, (ClientData) topLevel,
	    (void (*)_ANSI_ARGS_((ClientData))) NULL);

    /*
     * Parse options database for fontSets, schemes, etc
     */

    if (ParseToolkitOptions(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    /*
     * In normal operation mode, we use the command defined in
     * tixInitScript to load the Tix library scripts off the file
     * system
     */

    return Tcl_EvalEx(interp, initScript, -1, TCL_GLOBAL_ONLY);
}

/*----------------------------------------------------------------------
 * Tix_SafeInit --
 *
 * 	Initializes Tix for a safe interpreter.
 *
 *      TODO: the week security check in Tix is probably not complete
 *      and may lead to security holes. This function is temporarily
 *      disabled.
 *
 * Results:
 *	A standard Tcl completion code (TCL_OK or TCL_ERROR).  Also
 *	leaves information in the interp's result.
 *
 * Side effects:
 *	Sets "tix_library" Tcl variable, runs "Init.tcl" script from
 *      the Tix script library directory.
 * 
 *----------------------------------------------------------------------
 */

int
Tix_SafeInit(interp)
    Tcl_Interp * interp;
{
#if 0
    Tcl_SetVar2(interp, "tix_priv", "isSafe", "1", TCL_GLOBAL_ONLY);
    return Tix_Init(interp);
#else
    Tcl_AppendResult(interp, "Tix has not been tested for use in a safe ",
            "interppreter. Modify tixInit.c at your own risks", NULL);
    return TCL_ERROR;
#endif
}

