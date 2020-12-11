/*
 * tixCmds.c --
 *
 *	Implements various TCL commands for Tix. This file contains
 *	misc small commands, or groups of commands, that are not big
 *	enough to be put in a separate file.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000	   Tix Project Group.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * $Id: tixCmds.c,v 1.4 2004/03/28 02:44:56 hobbs Exp $
 */

#include <tixPort.h>
#include <tixInt.h>
#include <tixDef.h>
#include <math.h>

TIX_DECLARE_CMD(Tix_ParentWindow);

/*
 * Maximum intensity for a color:
 */

#define MAX_INTENSITY ((int) 65535)

/*
 * Data structure used by the tixDoWhenIdle command.
 */
typedef struct {
    Tcl_Interp * interp;
    char       * command;
    Tk_Window  tkwin;
} IdleStruct;

/*
 * Data structures used by the tixDoWhenMapped command.
 */
typedef struct _MapCmdLink {
    char * command;
    struct _MapCmdLink * next;
} MapCmdLink;

typedef struct {
    Tcl_Interp * interp;
    Tk_Window	 tkwin;
    MapCmdLink * cmds;
} MapEventStruct;

/*
 * Global vars
 */
static Tcl_HashTable idleTable;		/* hash table for TixDoWhenIdle */
static Tcl_HashTable mapEventTable;	/* hash table for TixDoWhenMapped */


/*
 * Functions used only in this file.
 */
static void		IdleHandler _ANSI_ARGS_((ClientData clientData));
static void		EventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		MapEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		IsOption _ANSI_ARGS_((CONST84 char *option,
			    int optArgc, CONST84 char **optArgv));
static XColor *		ScaleColor _ANSI_ARGS_((Tk_Window tkwin,
			    XColor * color, double scale));
static char *		NameOfColor _ANSI_ARGS_((XColor * colorPtr));


/*----------------------------------------------------------------------
 * Tix_DoWhenIdle --
 *
 *	The difference between "tixDoWhenIdle" and "after" is: the
 *	"after" handler is called after all other TK Idel Event
 *	Handler are called.  Sometimes this will cause some toplevel
 *	windows to be mapped before the Idle Event Handler is
 *	executed.
 *
 *	This behavior of "after" is not suitable for implementing
 *	geometry managers. Therefore I wrote "tixDoWhenIdle" which is
 *	an exact TCL interface for Tk_DoWhenIdle()
 *----------------------------------------------------------------------
 */

TIX_DEFINE_CMD(Tix_DoWhenIdleCmd)
{
    int			isNew;
    char	      * command;
    static int		inited = 0;
    IdleStruct	      * iPtr;
    Tk_Window		tkwin;
    Tcl_HashEntry * hashPtr;
 
    if (!inited) {
	Tcl_InitHashTable(&idleTable, TCL_STRING_KEYS);
	inited = 1;
    }

    /*
     * parse the arguments
     */
    if (strncmp(argv[0], "tixWidgetDoWhenIdle", strlen(argv[0]))== 0) {
	if (argc<3) {
	    return Tix_ArgcError(interp, argc, argv, 1,
		"command window ?arg arg ...?");
	}
	/* tixWidgetDoWhenIdle reqires that the second argument must
	 * be the name of a mega widget
	 */
	tkwin=Tk_NameToWindow(interp, argv[2], Tk_MainWindow(interp));
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    } else {
	if (argc<2) {
	    return Tix_ArgcError(interp, argc, argv, 1,
		"command ?arg arg ...?");
	}
	tkwin = NULL;
    }

    command = Tcl_Merge(argc-1, argv+1);

    hashPtr = Tcl_CreateHashEntry(&idleTable, command, &isNew);

    if (!isNew) {
	ckfree(command);
    } else {
	iPtr = (IdleStruct *) ckalloc(sizeof(IdleStruct));
	iPtr->interp  = interp;
	iPtr->command = command;
	iPtr->tkwin = tkwin;

	Tcl_SetHashValue(hashPtr, (char*)iPtr);

	if (tkwin) {
	    /* we just want one event handler for all idle events
	     * associated with a window. This is done by first calling
	     * Delete and then Create EventHandler.
	     */
	    Tk_DeleteEventHandler(tkwin, StructureNotifyMask, EventProc,
		(ClientData)tkwin);
	    Tk_CreateEventHandler(tkwin, StructureNotifyMask, EventProc,
		(ClientData)tkwin);
	}

	Tk_DoWhenIdle(IdleHandler, (ClientData) iPtr);
    }

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Tix_DoWhenMapped
 *
 *	Arranges a command to be called when the window received an
 *	<Map> event.
 *
 * argv[1..] = command argvs
 *
 *----------------------------------------------------------------------
 */
TIX_DEFINE_CMD(Tix_DoWhenMappedCmd)
{
    Tcl_HashEntry     * hashPtr;
    int			isNew;
    MapEventStruct    * mPtr;
    MapCmdLink	      * cmd;
    Tk_Window		tkwin;
    static int		inited = 0;

    if (argc!=3) {
	return Tix_ArgcError(interp, argc, argv, 1, " pathname command");
    }

    tkwin = Tk_NameToWindow(interp, argv[1], Tk_MainWindow(interp));
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    if (!inited) {
	Tcl_InitHashTable(&mapEventTable, TCL_ONE_WORD_KEYS);
	inited = 1;
    }

    hashPtr = Tcl_CreateHashEntry(&mapEventTable, (char*)tkwin, &isNew);

    if (!isNew) {
	mPtr = (MapEventStruct*) Tcl_GetHashValue(hashPtr);
    } else {
	mPtr = (MapEventStruct*) ckalloc(sizeof(MapEventStruct));
	mPtr->interp = interp;
	mPtr->tkwin  = tkwin;
	mPtr->cmds   = 0;

	Tcl_SetHashValue(hashPtr, (char*)mPtr);

	Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    MapEventProc, (ClientData)mPtr);
    }

    /*
     * Add this into a link list
     */
    cmd = (MapCmdLink*) ckalloc(sizeof(MapCmdLink));
    cmd->command = tixStrDup(argv[2]);

    cmd->next = mPtr->cmds;
    mPtr->cmds = cmd;

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Tix_Get3DBorderCmd
 *
 *	Returns the upper and lower border shades of a color. Returns then
 *	in a list of two X color names.
 *
 *	The color is not very useful if the display is a mono display:
 *	it will just return black and white. So a clever program may
 *	want to check the [tk colormodel] and if it is mono, then
 *	dither using a bitmap.
 *----------------------------------------------------------------------
 */
TIX_DEFINE_CMD(Tix_Get3DBorderCmd)
{
    XColor * color, * light, * dark;
    Tk_Window tkwin;
    Tk_Uid colorUID;

    if (argc != 2) {
	return Tix_ArgcError(interp, argc, argv, 0, "colorName");
    }

    tkwin = Tk_MainWindow(interp);

    colorUID = Tk_GetUid(argv[1]);
    color = Tk_GetColor(interp, tkwin, colorUID);
    if (color == NULL) {
	return TCL_ERROR;
    }

    if ((light = ScaleColor(tkwin, color, 1.4)) == NULL) {
	return TCL_ERROR;
    }
    if ((dark  = ScaleColor(tkwin, color, 0.6)) == NULL) {
	return TCL_ERROR;
    }

    Tcl_ResetResult(interp);
    Tcl_AppendElement(interp, NameOfColor(light));
    Tcl_AppendElement(interp, NameOfColor(dark));

    Tk_FreeColor(color);
    Tk_FreeColor(light);
    Tk_FreeColor(dark);

    return TCL_OK;
}

/*----------------------------------------------------------------------
 * Tix_HandleOptionsCmd
 *
 * 
 * argv[1] = recordName
 * argv[2] = validOptions
 * argv[3] = argList
 *	     if (argv[3][0] == "-nounknown") then 
 *		don't complain about unknown options
 *----------------------------------------------------------------------
 */
TIX_DEFINE_CMD(Tix_HandleOptionsCmd)
{
    int		listArgc;
    int		optArgc;
    CONST84 char ** listArgv = 0;
    CONST84 char ** optArgv  = 0;
    int		i, code = TCL_OK;
    int		noUnknown = 0;

    if (argc >= 2 && (strcmp(argv[1], "-nounknown") == 0)) {
	noUnknown = 1;
	argv[1] = argv[0];
	argc --;
	argv ++;
    }

    if (argc!=4) {
	return Tix_ArgcError(interp, argc, argv, 2, "w validOptions argList");
    }

    if (Tcl_SplitList(interp, argv[2], &optArgc,  &optArgv ) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }
    if (Tcl_SplitList(interp, argv[3], &listArgc, &listArgv) != TCL_OK) {
	code = TCL_ERROR;	
	goto done;
    }

    if ((listArgc %2) == 1) {
	if (noUnknown || IsOption(listArgv[listArgc-1], optArgc, optArgv)) {
	    Tcl_AppendResult(interp, "value for \"", listArgv[listArgc-1],
		"\" missing", (char*)NULL);
	} else {
	    Tcl_AppendResult(interp, "unknown option \"", listArgv[listArgc-1],
		"\"", (char*)NULL);
	}
	code = TCL_ERROR;
	goto done;
    }
    for (i=0; i<listArgc; i+=2) {
	if (IsOption(listArgv[i], optArgc, optArgv)) {
	    Tcl_SetVar2(interp, argv[1], listArgv[i], listArgv[i+1], 0);
	}
	else if (!noUnknown) {
	    Tcl_AppendResult(interp, "unknown option \"", listArgv[i],
		"\"; must be one of \"", argv[2], "\".", NULL);
	    code = TCL_ERROR;
	    goto done;
	}
    }

  done:

    if (listArgv) {
	ckfree((char *) listArgv);
    }
    if (optArgv) {
	ckfree((char *) optArgv);
    }

    return code;
}

/*----------------------------------------------------------------------
 * Tix_SetWindowParent --
 *
 *	Sets the parent of a window. This is normally to change the
 *	state of toolbar and MDI windows between docking and free
 *	modes.
 *
 * Results:
 *	Standard Tcl results.
 *
 * Side effects:
 *	Windows may be re-parented. See user documentation.
 *----------------------------------------------------------------------
 */

TIX_DEFINE_CMD(Tix_ParentWindow)
{
    Tk_Window mainWin, tkwin, newParent;
    int parentId;

    if (argc != 3) {
	return Tix_ArgcError(interp, argc, argv, 1, "window parent");
    }
    mainWin = Tk_MainWindow(interp);
    if (mainWin == NULL) {
	Tcl_SetResult(interp, "interpreter does not have a main window",
	    TCL_STATIC);
	return TCL_ERROR;
    }

    tkwin = Tk_NameToWindow(interp, argv[1], mainWin);
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    newParent = Tk_NameToWindow(interp, argv[2], mainWin);
    if (newParent == NULL) {
	if (Tcl_GetInt(interp, argv[2], &parentId) != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp, "\"", argv[2],
		"\" must be a window pathname or ID", NULL);
	    return TCL_ERROR;
	}
    }

    return TixpSetWindowParent(interp, tkwin, newParent, parentId);
}

/*----------------------------------------------------------------------
 * Tix_TmpLineCmd
 *
 *	Draw a temporary line on the root window
 *
 * argv[1..] = x1 y1 x2 y2
 *----------------------------------------------------------------------
 */
TIX_DEFINE_CMD(Tix_TmpLineCmd)
{
    Tk_Window mainWin = (Tk_Window)clientData;
    Tk_Window tkwin;
    int x1, y1, x2, y2;
    
    if (argc != 5 && argc != 6) {
	return Tix_ArgcError(interp, argc, argv, 0,
	    "tixTmpLine x1 y1 x2 y2 ?window?");
    }
    if (Tcl_GetInt(interp, argv[1], &x1) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &y1) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[3], &x2) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[4], &y2) != TCL_OK) {
	return TCL_ERROR;
    }
    if (argc == 6) {
	/*
	 * argv[5] tells which display to draw the tmp lines on, in
	 * case the application has opened more than one displays. If
	 * this argv[5] is omitted, draws to the display where the
	 * main window is on.
	 */
	tkwin = Tk_NameToWindow(interp, argv[5], mainWin);
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}
    } else {
	tkwin = Tk_MainWindow(interp);
    }

    TixpDrawTmpLine(x1, y1, x2, y2, tkwin);
    return TCL_OK;
}

/*----------------------------------------------------------------------
 * EventProc --
 *
 *	Monitors events sent to a window associated with a
 *	tixWidgetDoWhenIdle command. If this window is destroyed,
 *	remove the idle handlers associated with this window.
 *----------------------------------------------------------------------
 */

static void EventProc(clientData, eventPtr)
    ClientData clientData;
    XEvent *eventPtr;
{
    Tk_Window tkwin = (Tk_Window)clientData;
    Tcl_HashSearch hSearch;
    Tcl_HashEntry * hashPtr;
    IdleStruct * iPtr;

    if (eventPtr->type != DestroyNotify) {
	return;
    }

    /* Iterate over all the entries in the hash table */
    for (hashPtr = Tcl_FirstHashEntry(&idleTable, &hSearch);
	 hashPtr;
	 hashPtr = Tcl_NextHashEntry(&hSearch)) {

	iPtr = (IdleStruct *)Tcl_GetHashValue(hashPtr);

	if (iPtr->tkwin == tkwin) {
	    Tcl_DeleteHashEntry(hashPtr);
	    Tk_CancelIdleCall(IdleHandler, (ClientData) iPtr);
	    ckfree((char*)iPtr->command);
	    ckfree((char*)iPtr);
	}
    }
}
/*----------------------------------------------------------------------
 * IdleHandler --
 *
 *	Called when Tk is idle. Dispatches all commands registered by
 *	tixDoWhenIdle and tixWidgetDoWhenIdle.
 *----------------------------------------------------------------------
 */

static void IdleHandler(clientData)
    ClientData clientData;	/* TCL command to evaluate */
{
    Tcl_HashEntry * hashPtr;
    IdleStruct * iPtr;

    iPtr = (IdleStruct *) clientData;

    /*
     * Clean up the hash table. Note that we have to do this BEFORE
     * calling the TCL command. Otherwise if the TCL command tries
     * to register itself again, it will fail in Tix_DoWhenIdleCmd()
     * because the command is still in the hashtable
     */
    hashPtr = Tcl_FindHashEntry(&idleTable, iPtr->command);
    if (hashPtr) {
	Tcl_DeleteHashEntry(hashPtr);
    } else {
	/* Probably some kind of error */
	return;
    }

    if (Tcl_GlobalEval(iPtr->interp, iPtr->command) != TCL_OK) {
	if (iPtr->tkwin != NULL) {
	    Tcl_AddErrorInfo(iPtr->interp,
		"\n    (idle event handler executed by tixWidgetDoWhenIdle)");
	} else {
	    Tcl_AddErrorInfo(iPtr->interp,
		"\n    (idle event handler executed by tixDoWhenIdle)");
	} 
	Tk_BackgroundError(iPtr->interp);
    }

    ckfree((char*)iPtr->command);
    ckfree((char*)iPtr);
}

/*----------------------------------------------------------------------
 * IsOption --
 *
 *	Checks whether the string pointed by "option" is one of the
 *	options given by the "optArgv" array.
 *----------------------------------------------------------------------
 */
static int IsOption(option, optArgc, optArgv)
    CONST84 char *option;	/* Number of arguments. */ 
    int optArgc;		/* Number of arguments. */
    CONST84 char **optArgv;	/* Argument strings. */
{
    int i;

    for (i=0; i<optArgc; i++) {
	if (strcmp(option, optArgv[i]) == 0) {
	    return 1;
	}
    }
    return 0;
}


static void MapEventProc(clientData, eventPtr)
    ClientData clientData;	/* TCL command to evaluate */
    XEvent *eventPtr;		/* Information about event. */
{
    Tcl_HashEntry     * hashPtr;
    MapEventStruct    * mPtr;
    MapCmdLink	      * cmd;

    if (eventPtr->type != MapNotify) {
	return;
    }

    mPtr = (MapEventStruct *) clientData;

    Tk_DeleteEventHandler(mPtr->tkwin, StructureNotifyMask,
	MapEventProc, (ClientData)mPtr);

    /* Clean up the hash table.
     */
    if ((hashPtr = Tcl_FindHashEntry(&mapEventTable, (char*)mPtr->tkwin))) {
	Tcl_DeleteHashEntry(hashPtr);
    }

    for (cmd = mPtr->cmds; cmd; ) {
	MapCmdLink * old;

	/* Execute the event handler */
	if (Tcl_GlobalEval(mPtr->interp, cmd->command) != TCL_OK) {
	    Tcl_AddErrorInfo(mPtr->interp,
		"\n    (event handler executed by tixDoWhenMapped)");
	    Tk_BackgroundError(mPtr->interp);
	}

	/* Delete the link */
	old = cmd;
	cmd = cmd->next;

	ckfree(old->command);
	ckfree((char*)old);
    }

    /* deallocate the mapEventStruct */
    ckfree((char*)mPtr);
}

static char *
NameOfColor(colorPtr)
   XColor * colorPtr;
{
    static char string[20];
    char *ptr;

    sprintf(string, "#%4x%4x%4x", colorPtr->red, colorPtr->green,
	colorPtr->blue);

    for (ptr = string; *ptr; ptr++) {
	if (*ptr == ' ') {
	    *ptr = '0';
	}
    }
    return string;
}


static XColor *
ScaleColor(tkwin, color, scale)
    Tk_Window tkwin;
    XColor * color;
    double scale;
{
    int r, g, b;
    XColor test;

    r = (int)((float)(color->red)   * scale);
    g = (int)((float)(color->green) * scale);
    b = (int)((float)(color->blue)  * scale);
    if (r > MAX_INTENSITY) { r = MAX_INTENSITY; }
    if (g > MAX_INTENSITY) { g = MAX_INTENSITY; }
    if (b > MAX_INTENSITY) { b = MAX_INTENSITY; }
    test.red   = (unsigned short) r;
    test.green = (unsigned short) g;
    test.blue  = (unsigned short) b;

    return Tk_GetColorByValue(tkwin, &test);
}

/*----------------------------------------------------------------------
 * Tix_GetDefaultCmd
 *
 *	Implements the tixGetDefault command.
 *
 *	Returns values for various default configuration options,
 *	as defined in tixDef.h and tkDefault.h.
 *
 *	This command makes it possible to define default options
 *	for the Tcl-implemented widgets according to platform-
 *	specific values set in the C header files.
 *
 *	Note: there is no reason to make this an ObjCmd because the
 *	same Tcl command is unlikely to be executed twice. The old
 *	style "string" command is more compact and less prone to
 *	programming errors.
 *----------------------------------------------------------------------
 */

TIX_DEFINE_CMD(Tix_GetDefaultCmd)
{
    unsigned int i;
#   define OPT(x) {#x, x}
    static char *table[][2] = {
	OPT(ACTIVE_BG),
	OPT(CTL_FONT),
	OPT(DISABLED),
	OPT(HIGHLIGHT),
	OPT(INDICATOR),
	OPT(MENU_BG),
	OPT(MENU_FG),
	OPT(NORMAL_BG),
	OPT(NORMAL_FG),
	OPT(SELECT_BG),
	OPT(SELECT_FG),
	OPT(TEXT_FG),
	OPT(TROUGH),
	
	OPT(TIX_EDITOR_BG),
	OPT(TIX_BORDER_WIDTH),
	OPT(TIX_HIGHLIGHT_THICKNESS),
    };

    if (argc != 2) {
	return Tix_ArgcError(interp, argc, argv, 1, "optionName");
    }

    for (i=0; i<Tix_ArraySize(table); i++) {
	if (strcmp(argv[1], table[i][0]) == 0) {
	    Tcl_SetResult(interp,   table[i][1], TCL_STATIC);
	    return TCL_OK;
	}
    }

    Tcl_AppendResult(interp, "unknown option \"", argv[1], 
	    "\"", NULL);
    return TCL_ERROR;
}
