
/*	$Id: tixUnixMwm.c,v 1.2 2005/03/25 20:15:53 hobbs Exp $	*/

/*
 * tixUnixMwm.c --
 *
 *	Communicating with the Motif window manager.
 *
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <tkInt.h>
#include <tixPort.h>
#include <tixInt.h>
#ifndef MAC_OSX_TK
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>


#ifdef HAS_MOTIF_INC
#include <Xm/MwmUtil.h>
#else

/*
 * This section is provided for the machines that don't have the Motif
 * header files installed.
 */

#define MWM_DECOR_ALL           (1L << 0)
#define MWM_DECOR_BORDER        (1L << 1)
#define MWM_DECOR_RESIZEH       (1L << 2)
#define MWM_DECOR_TITLE         (1L << 3)
#define MWM_DECOR_MENU          (1L << 4)
#define MWM_DECOR_MINIMIZE      (1L << 5)
#define MWM_DECOR_MAXIMIZE      (1L << 6)

#define MWM_HINTS_DECORATIONS   (1L << 1)

#define PROP_MOTIF_WM_HINTS_ELEMENTS    5
#define PROP_MWM_HINTS_ELEMENTS         PROP_MOTIF_WM_HINTS_ELEMENTS

/* atom name for _MWM_HINTS property */
#define _XA_MOTIF_WM_HINTS      "_MOTIF_WM_HINTS"
#define _XA_MWM_HINTS           _XA_MOTIF_WM_HINTS

#define _XA_MOTIF_WM_MENU       "_MOTIF_WM_MENU"
#define _XA_MWM_MENU            _XA_MOTIF_WM_MENU

#define _XA_MOTIF_WM_INFO       "_MOTIF_WM_INFO"
#define _XA_MWM_INFO            _XA_MOTIF_WM_INFO

#define PROP_MOTIF_WM_INFO_ELEMENTS     2
#define PROP_MWM_INFO_ELEMENTS          PROP_MOTIF_WM_INFO_ELEMENTS

typedef struct
{
    CARD32      flags;
    CARD32      functions;
    CARD32      decorations;
    INT32       inputMode;
    CARD32      status;
} PropMotifWmHints;

typedef PropMotifWmHints        PropMwmHints;

typedef struct
{
    CARD32 flags;
    CARD32 wmWindow;
} PropMotifWmInfo;

typedef PropMotifWmInfo PropMwmInfo;
 
#endif	/* HAS_MOTIF_INC */

#define MWM_DECOR_UNKNOWN (-1)
#define MWM_DECOR_EVERYTHING (MWM_DECOR_BORDER |\
			      MWM_DECOR_RESIZEH |\
			      MWM_DECOR_TITLE |\
			      MWM_DECOR_MENU |\
			      MWM_DECOR_MINIMIZE |\
			      MWM_DECOR_MAXIMIZE)

typedef struct _Tix_MwmInfo {
    Tcl_Interp	      * interp;
    Tk_Window 		tkwin;
    PropMwmHints	prop;		/* not used */
    Atom 		mwm_hints_atom;
    Tcl_HashTable 	protocols;
    unsigned int	isremapping : 1;
    unsigned int	resetProtocol : 1;
    unsigned int	addedMwmMsg : 1;
} Tix_MwmInfo;

typedef struct Tix_MwmProtocol {
    Atom 		protocol;
    char 	      * name;
    char 	      * menuMessage;
    size_t 		messageLen;
    unsigned int	active : 1;
} Tix_MwmProtocol;


/* Function declaration */

static void		AddMwmProtocol _ANSI_ARGS_((Tcl_Interp *interp,
			    Tix_MwmInfo *wmPtr, CONST84 char * name, CONST84 char * message));
static void		ActivateMwmProtocol _ANSI_ARGS_((Tcl_Interp *interp,
			    Tix_MwmInfo *wmPtr, CONST84 char * name));
static void		DeactivateMwmProtocol _ANSI_ARGS_((Tcl_Interp *interp,
			    Tix_MwmInfo *wmPtr, CONST84 char * name));
static void		DeleteMwmProtocol _ANSI_ARGS_((Tcl_Interp *interp,
			    Tix_MwmInfo *wmPtr, CONST84 char * name));
static Tix_MwmInfo *	GetMwmInfo _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin));
static Tix_MwmProtocol*	GetMwmProtocol _ANSI_ARGS_((Tcl_Interp * interp,
			    Tix_MwmInfo * wmPtr, Atom protocol));
static int		IsMwmRunning _ANSI_ARGS_((Tcl_Interp * interp,
			    Tix_MwmInfo*wmPtr));
static int 		MwmDecor _ANSI_ARGS_((Tcl_Interp * interp,
			    CONST84 char * string));
static int		MwmProtocol _ANSI_ARGS_((Tcl_Interp * interp,
			    Tix_MwmInfo * wmPtr, int argc, CONST84 char ** argv));
static void		QueryMwmHints _ANSI_ARGS_((Tix_MwmInfo * wmPtr));
static void		RemapWindow _ANSI_ARGS_((ClientData clientData));
static void		RemapWindowWhenIdle _ANSI_ARGS_((
			    Tix_MwmInfo * wmPtr));
static void 		ResetProtocols _ANSI_ARGS_((ClientData clientData));
static void		ResetProtocolsWhenIdle _ANSI_ARGS_((
			    Tix_MwmInfo * wmPtr));
static int 		SetMwmDecorations _ANSI_ARGS_((Tcl_Interp *interp,
			    Tix_MwmInfo*wmPtr, int argc, CONST84 char ** argv));
static int		SetMwmTransientFor _ANSI_ARGS_((Tcl_Interp *interp,
			    Tix_MwmInfo*wmPtr, TkWindow *mainWindow, int argc,
			    CONST84 char ** argv));
static void		StructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));

/* Local variables */

static Tcl_HashTable mwmTable;


/*
 *----------------------------------------------------------------------
 *
 * Tix_MwmCmd --
 *
 *	This procedure is invoked to process the "mwm" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

/* ARGSUSED */
int
Tix_MwmCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    CONST84 char **argv;	/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    TkWindow *winPtr;
    char c;
    size_t length;
    Tix_MwmInfo * wmPtr;

    if (argc < 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option pathname ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);

    if (!(winPtr = (TkWindow *) Tk_NameToWindow(interp, argv[2], tkwin))) {
	return TCL_ERROR;
    }
    if (!Tk_IsTopLevel(winPtr)) {
	Tcl_AppendResult(interp, argv[2], " is not a toplevel window.", NULL);
	return TCL_ERROR;
    }
    if (!(wmPtr=GetMwmInfo(interp, (Tk_Window) winPtr))) {
	return TCL_ERROR;
    }

    if ((c == 'd') && (strncmp(argv[1], "decorations", length) == 0)) {
	return SetMwmDecorations(interp, wmPtr, argc-3, argv+3);
    }
    else if ((c == 'i') && (strncmp(argv[1], "ismwmrunning", length) == 0)) {
	if (IsMwmRunning(interp, wmPtr)) {
	    Tcl_AppendResult(interp, "1", NULL);
	} else {
	    Tcl_AppendResult(interp, "0", NULL);
	}
	return TCL_OK;
    }
    else if ((c == 'p') && (strncmp(argv[1], "protocol", length) == 0)) {
	return MwmProtocol(interp, wmPtr, argc-3, argv+3);
    }
    else if ((c == 't') && (strncmp(argv[1], "transientfor", length) == 0)) {
	return SetMwmTransientFor(interp, wmPtr, winPtr, argc-3, argv+3);
    }
    else {
	Tcl_AppendResult(interp, "unknown or ambiguous option \"",
	    argv[1], "\": must be decorations, ismwmrunning, protocol ",
	    "or transientfor",
	    NULL);
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 * TixMwmProtocolHandler --
 *
 *	A generic X event handler that handles the events from the Mwm
 *	Window manager.
 *
 * Results:
 *	True iff the event has been handled.
 *
 * Side effects:
 *	None.
 *----------------------------------------------------------------------
 */

int
TixMwmProtocolHandler(clientData, eventPtr)
    ClientData clientData;
    XEvent *eventPtr;
{
    TkWindow *winPtr;
    Window handlerWindow;

    if (eventPtr->type != ClientMessage) {
	return 0;
    }

    handlerWindow = eventPtr->xany.window;
    winPtr = (TkWindow *) Tk_IdToWindow(eventPtr->xany.display, handlerWindow);
    if (winPtr != NULL) {
	if (eventPtr->xclient.message_type ==
	    Tk_InternAtom((Tk_Window) winPtr,"_MOTIF_WM_MESSAGES")) {
	    TkWmProtocolEventProc(winPtr, eventPtr);
	    return 1;
	}
    }
    return 0;
}

static int
MwmDecor(interp, string)
    Tcl_Interp * interp;
    CONST84 char * string;
{
    size_t len = strlen(string);

    if (strncmp(string, "-all", len) == 0) {
	return MWM_DECOR_ALL;
    } else if (strncmp(string, "-border", len) == 0) {
	return MWM_DECOR_BORDER;
    } else if (strncmp(string, "-resizeh", len) == 0) {
	return MWM_DECOR_RESIZEH;
    } else if (strncmp(string, "-title", len) == 0) {
	return MWM_DECOR_TITLE;
    } else if (strncmp(string, "-menu", len) == 0) {
	return MWM_DECOR_MENU;
    } else if (strncmp(string, "-minimize", len) == 0) {
	return MWM_DECOR_MINIMIZE;
    } else if (strncmp(string, "-maximize", len) == 0) {
	return MWM_DECOR_MAXIMIZE;
    } else {
	Tcl_AppendResult(interp, "unknown decoration \"", string, "\"", NULL);
	return -1;
    }
}


static void
QueryMwmHints(wmPtr)
    Tix_MwmInfo * wmPtr;
{
    Atom 		actualType;
    int 		actualFormat;
    unsigned long 	numItems, bytesAfter;

    wmPtr->prop.flags = MWM_HINTS_DECORATIONS;

    if (XGetWindowProperty(Tk_Display(wmPtr->tkwin),Tk_WindowId(wmPtr->tkwin), 
	wmPtr->mwm_hints_atom, 0, PROP_MWM_HINTS_ELEMENTS,
	False, wmPtr->mwm_hints_atom, &actualType, &actualFormat, &numItems,
	&bytesAfter, (unsigned char **) & wmPtr->prop) == Success) {

	if ((actualType != wmPtr->mwm_hints_atom) || (actualFormat != 32) ||
	    (numItems <= 0)) {
	    /* It looks like this window doesn't have a _XA_MWM_HINTS
	     * property. Let's give the default value
	     */
	    wmPtr->prop.decorations = MWM_DECOR_EVERYTHING;
	}
    } else {
	/* We get an error somehow. Pretend that the decorations are all
	 */
	wmPtr->prop.decorations = MWM_DECOR_EVERYTHING;
    }
}

static void
RemapWindow(clientData)
    ClientData clientData;
{
    Tix_MwmInfo * wmPtr = (Tix_MwmInfo *)clientData;

    Tk_UnmapWindow(wmPtr->tkwin);
    Tk_MapWindow(wmPtr->tkwin);
    wmPtr->isremapping = 0;
}

static void
RemapWindowWhenIdle(wmPtr)
    Tix_MwmInfo * wmPtr;
{
    if (!wmPtr->isremapping) {
	wmPtr->isremapping = 1;
	Tk_DoWhenIdle(RemapWindow, (ClientData)wmPtr);
    }
}

/*
 * SetMwmDecorations --
 *
 *
 */
static
int SetMwmDecorations(interp, wmPtr, argc, argv)
    Tcl_Interp *interp;
    Tix_MwmInfo*wmPtr;
    int 	argc;
    CONST84 char     ** argv;
{
    int			i;
    int			decor;
    char		buff[40];

    if (argc == 0 || argc == 1) {
	/*
	 * Query the existing settings
	 */
	QueryMwmHints(wmPtr);

	if (argc == 0) {
	    /*
	     * Query all hints
	     */
	    sprintf(buff, "-border %d", 
		((wmPtr->prop.decorations & MWM_DECOR_BORDER)!=0));
	    Tcl_AppendElement(interp, buff);

	    sprintf(buff, "-resizeh %d", 
		((wmPtr->prop.decorations &MWM_DECOR_RESIZEH)!=0));
	    Tcl_AppendElement(interp, buff);
    
	    sprintf(buff, "-title %d", 
		((wmPtr->prop.decorations & MWM_DECOR_TITLE)!=0));
	    Tcl_AppendElement(interp, buff);
    
	    sprintf(buff, "-menu %d", 
		((wmPtr->prop.decorations & MWM_DECOR_MENU)!=0));
	    Tcl_AppendElement(interp, buff);
    
	    sprintf(buff, "-minimize %d", 
		((wmPtr->prop.decorations&MWM_DECOR_MINIMIZE)!=0));
	    Tcl_AppendElement(interp, buff);
    
	    sprintf(buff, "-maximize %d", 
		((wmPtr->prop.decorations&MWM_DECOR_MAXIMIZE)!=0));
	    Tcl_AppendElement(interp, buff);

	    return TCL_OK;
	} else {
	    /*
	     * Query only one hint
	     */
	    if ((decor = MwmDecor(interp, argv[0])) == MWM_DECOR_UNKNOWN) {
		return TCL_ERROR;
	    }

	    if (wmPtr->prop.decorations & decor) {
		Tcl_AppendResult(interp, "1", NULL);
	    } else {
		Tcl_AppendResult(interp, "0", NULL);
	    }
	    return TCL_OK;
	}
    } else {
	if (argc %2) {
	    Tcl_AppendResult(interp, "value missing for option \"",
		argv[argc-1], "\"", NULL);
		return TCL_ERROR;
	}

	for (i=0; i<argc; i+=2) {
	    int value;

	    if ((decor = MwmDecor(interp, argv[i])) == MWM_DECOR_UNKNOWN)  {
		return TCL_ERROR;
	    }

	    if (Tcl_GetBoolean(interp, argv[i+1], &value) != TCL_OK) {
		return TCL_ERROR;
	    }

	    if (value) {
		wmPtr->prop.decorations |= decor;
	    }
	    else {
		wmPtr->prop.decorations &= ~decor;
	    }

	    if (decor == MWM_DECOR_ALL) {
		if (value) {
		    wmPtr->prop.decorations |= MWM_DECOR_EVERYTHING;
		} else {
		    wmPtr->prop.decorations &= ~MWM_DECOR_EVERYTHING;
		}
	    }
	}

	wmPtr->prop.flags = MWM_HINTS_DECORATIONS;
	XChangeProperty(Tk_Display(wmPtr->tkwin), Tk_WindowId(wmPtr->tkwin),
	    wmPtr->mwm_hints_atom, wmPtr->mwm_hints_atom, 32, PropModeReplace,
	    (unsigned char *) &wmPtr->prop, PROP_MWM_HINTS_ELEMENTS);

	if (Tk_IsMapped(wmPtr->tkwin)) {
	    /* Needs unmap/map to refresh */
	    RemapWindowWhenIdle(wmPtr);
	}
	return TCL_OK;
    }
}

static int MwmProtocol(interp, wmPtr, argc, argv)
    Tcl_Interp * interp;
    Tix_MwmInfo * wmPtr;
    int argc;
    CONST84 char ** argv;
{
    size_t len;

    if (argc == 0) {
	Tcl_HashSearch 	  hSearch;
	Tcl_HashEntry 	* hashPtr;
	Tix_MwmProtocol * ptPtr;

	/* Iterate over all the entries in the hash table */
	for (hashPtr = Tcl_FirstHashEntry(&wmPtr->protocols, &hSearch);
	     hashPtr;
	     hashPtr = Tcl_NextHashEntry(&hSearch)) {

	    ptPtr = (Tix_MwmProtocol *)Tcl_GetHashValue(hashPtr);
	    Tcl_AppendElement(interp, ptPtr->name);
	}
	return TCL_OK;
    }

    len = strlen(argv[0]);
    if (strncmp(argv[0], "add", len) == 0 && argc == 3) {
	AddMwmProtocol(interp, wmPtr, argv[1], argv[2]);
    }
    else if (strncmp(argv[0], "activate", len) == 0 && argc == 2) {
	ActivateMwmProtocol(interp, wmPtr, argv[1]);
    }
    else if (strncmp(argv[0], "deactivate", len) == 0 && argc == 2) {
	DeactivateMwmProtocol(interp, wmPtr, argv[1]);
    }
    else if (strncmp(argv[0], "delete", len) == 0 && argc == 2) {
	DeleteMwmProtocol(interp, wmPtr, argv[1]);
    }
    else {
	Tcl_AppendResult(interp, "unknown option \"", argv[0],
	    "\" should be add, activate, deactivate or delete", NULL);
	return TCL_ERROR;
    }

    return TCL_OK;
}


static void AddMwmProtocol(interp, wmPtr, name, message)
    Tcl_Interp  *interp;
    Tix_MwmInfo *wmPtr;
    CONST84 char * name;
    CONST84 char * message;
{
    Atom protocol;
    Tix_MwmProtocol *ptPtr;

    protocol = Tk_InternAtom(wmPtr->tkwin, name);
    ptPtr = GetMwmProtocol(interp, wmPtr, protocol);
    
    if (ptPtr->menuMessage != NULL) {
	/* This may happen if "protocol add" called twice for the same name */
	ckfree(ptPtr->menuMessage);
    }

    if (ptPtr->name == NULL) {
	ptPtr->name = tixStrDup(name);
    }
    ptPtr->menuMessage = tixStrDup(message);
    ptPtr->messageLen  = strlen(message);
    ptPtr->active = 1;

    ResetProtocolsWhenIdle(wmPtr);
}

static void ActivateMwmProtocol(interp, wmPtr, name)
    Tcl_Interp  *interp;
    Tix_MwmInfo *wmPtr;
    CONST84 char * name;
{
    Atom protocol;
    Tix_MwmProtocol *ptPtr;

    protocol = Tk_InternAtom(wmPtr->tkwin, name);
    ptPtr = GetMwmProtocol(interp, wmPtr, protocol);
    ptPtr->active = 1;

    ResetProtocolsWhenIdle(wmPtr);
}

static void DeactivateMwmProtocol(interp, wmPtr, name)
    Tcl_Interp  *interp;
    Tix_MwmInfo *wmPtr;
    CONST84 char * name;
{
    Atom protocol;
    Tix_MwmProtocol *ptPtr;

    protocol = Tk_InternAtom(wmPtr->tkwin, name);
    ptPtr = GetMwmProtocol(interp, wmPtr, protocol);
    ptPtr->active = 0;

    ResetProtocolsWhenIdle(wmPtr);
}

/*
 * Any "wm protocol" event handlers for the deleted protocol are
 * *not* automatically deleted. It is the application programmer's
 * responsibility to delete them using
 *
 *	wm protocol SOME_JUNK_PROTOCOL {}
 */
static void DeleteMwmProtocol(interp, wmPtr, name)
    Tcl_Interp  *interp;
    Tix_MwmInfo *wmPtr;
    CONST84 char * name;
{
    Atom protocol;
    Tix_MwmProtocol *ptPtr;
    Tcl_HashEntry * hashPtr;

    protocol = Tk_InternAtom(wmPtr->tkwin, name);
    hashPtr = Tcl_FindHashEntry(&wmPtr->protocols, (char*)protocol);

    if (hashPtr) {
	ptPtr = (Tix_MwmProtocol *)Tcl_GetHashValue(hashPtr);
	ckfree(ptPtr->name);
	ckfree(ptPtr->menuMessage);
	ckfree((char*)ptPtr);
	Tcl_DeleteHashEntry(hashPtr);
    }

    ResetProtocolsWhenIdle(wmPtr);
}


static void
ResetProtocolsWhenIdle(wmPtr)
    Tix_MwmInfo * wmPtr;
{
    if (!wmPtr->resetProtocol) {
	wmPtr->resetProtocol = 1;
	Tk_DoWhenIdle(ResetProtocols, (ClientData)wmPtr);
    }
}

static void ResetProtocols(clientData)
    ClientData clientData;
{
    Tix_MwmInfo       * wmPtr = (Tix_MwmInfo *) clientData;
    int 		numProtocols = wmPtr->protocols.numEntries;
    Atom 	      * atoms, mwm_menu_atom, motif_msgs;
    Tcl_HashSearch 	hSearch;
    Tcl_HashEntry     * hashPtr;
    Tix_MwmProtocol   * ptPtr;
    int			n;
    Tcl_DString		dString;

    atoms = (Atom*)ckalloc(numProtocols * sizeof(Atom));
    Tcl_DStringInit(&dString);

    /* Iterate over all the entries in the hash table */
    for (hashPtr = Tcl_FirstHashEntry(&wmPtr->protocols, &hSearch), n=0;
	 hashPtr;
	 hashPtr = Tcl_NextHashEntry(&hSearch)) {
	char tmp[100];

	ptPtr = (Tix_MwmProtocol *)Tcl_GetHashValue(hashPtr);
	if (ptPtr->active) {
	    atoms[n++] = ptPtr->protocol;
	}

	Tcl_DStringAppend(&dString, ptPtr->menuMessage, (int)ptPtr->messageLen);
	sprintf(tmp, " f.send_msg %d\n", (int)(ptPtr->protocol));
	Tcl_DStringAppend(&dString, tmp, (int)strlen(tmp));
    }

    /* Atoms for managing the MWM messages */
    mwm_menu_atom   = Tk_InternAtom(wmPtr->tkwin, _XA_MWM_MENU);
    motif_msgs      = Tk_InternAtom(wmPtr->tkwin, "_MOTIF_WM_MESSAGES");

    /* The _MOTIF_WM_MESSAGES atom must be in the wm_protocols. Otherwise
     * Mwm refuese to enable our menu items
     */
    if (!wmPtr->addedMwmMsg) {
	Tix_GlobalVarEval(wmPtr->interp, "wm protocol ",
	    Tk_PathName(wmPtr->tkwin), " _MOTIF_WM_MESSAGES {;}", NULL);
	wmPtr->addedMwmMsg = 1;
    }

    /*
     * These are the extra MWM protocols defined by this application.
     */
    XChangeProperty(Tk_Display(wmPtr->tkwin), Tk_WindowId(wmPtr->tkwin),
	motif_msgs, XA_ATOM, 32, PropModeReplace,
	(unsigned char *)atoms, n);

    /*
     * Update the MWM menu items
     */
    XChangeProperty(Tk_Display(wmPtr->tkwin), Tk_WindowId(wmPtr->tkwin),
	mwm_menu_atom, mwm_menu_atom, 8, PropModeReplace, 
	(unsigned char *)dString.string, dString.length+1);

    Tcl_DStringFree(&dString);
    ckfree((char*)atoms);

    /* Done ! */
    wmPtr->resetProtocol = 0;
    if (Tk_IsMapped(wmPtr->tkwin)) {
	/* Needs unmap/map to refresh */
	RemapWindowWhenIdle(wmPtr);
    }
}


static
int SetMwmTransientFor(interp, wmPtr, mainWindow, argc, argv)
    Tcl_Interp *interp;
    Tix_MwmInfo*wmPtr;
    TkWindow   *mainWindow;
    int 	argc;
    CONST84 char     ** argv;
{
    Atom 	transfor_atom;
    TkWindow  *	master;

    transfor_atom = Tk_InternAtom(wmPtr->tkwin, "WM_TRANSIENT_FOR");
    if (argc == 0) {
	return TCL_OK;
    } else if (argc == 1) {
	master = (TkWindow *) Tk_NameToWindow(interp, argv[0], 
	    (Tk_Window)mainWindow);
	if (master == NULL) {
	    return TCL_ERROR;
	}
	XChangeProperty(Tk_Display(wmPtr->tkwin), Tk_WindowId(wmPtr->tkwin),
	    transfor_atom, XA_WINDOW, 32, PropModeReplace,
	    (unsigned char *)&master->window, 1);
	return TCL_OK;
    } else {
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StructureProc --
 *
 *	Gets called in response to StructureNotify events in toplevels
 *	operated by the tixMwm command. 
 *
 * Results:
 *	none
 *
 * Side effects:
 *	The Tix_MwmInfo for the toplevel is deleted when the toplevel
 *	is destroyed.
 *
 *----------------------------------------------------------------------
 */
static void
StructureProc(clientData, eventPtr)
    ClientData clientData;		/* Our information about window
					 * referred to by eventPtr. */
    XEvent *eventPtr;			/* Describes what just happened. */
{
    register Tix_MwmInfo * wmPtr = (Tix_MwmInfo *) clientData;
    Tcl_HashEntry *hashPtr;
    
    if (eventPtr->type == DestroyNotify) {
	Tcl_HashSearch 	  hSearch;
	Tix_MwmProtocol * ptPtr;

	/* Delete all protocols in the hash table associated with
	 * this toplevel
	 */
	for (hashPtr = Tcl_FirstHashEntry(&wmPtr->protocols, &hSearch);
	     hashPtr;
	     hashPtr = Tcl_NextHashEntry(&hSearch)) {

	    ptPtr = (Tix_MwmProtocol *)Tcl_GetHashValue(hashPtr);
	    ckfree(ptPtr->name);
	    ckfree(ptPtr->menuMessage);
	    ckfree((char*)ptPtr);
	    Tcl_DeleteHashEntry(hashPtr);
	}

	Tcl_DeleteHashTable(&wmPtr->protocols);

	/*
	 * Delete info about this toplevel in the table of all toplevels
	 * controlled by tixMwm
	 */
	hashPtr = Tcl_FindHashEntry(&mwmTable, (char*)wmPtr->tkwin);
	if (hashPtr != NULL) {
	    Tcl_DeleteHashEntry(hashPtr);
	}

	if (wmPtr->resetProtocol) {
	    Tk_CancelIdleCall(ResetProtocols, (ClientData)wmPtr);
	    wmPtr->resetProtocol = 0;
	}

	ckfree((char*)wmPtr);
    }
}

static Tix_MwmInfo *
GetMwmInfo(interp, tkwin)
    Tcl_Interp * interp;
    Tk_Window tkwin;
{
    static inited = 0;
    Tcl_HashEntry *hashPtr;
    int isNew;

    if (!inited) {
	Tcl_InitHashTable(&mwmTable, TCL_ONE_WORD_KEYS);
	inited = 1;
    }

    hashPtr = Tcl_CreateHashEntry(&mwmTable, (char*)tkwin, &isNew);

    if (!isNew) {
	return (Tix_MwmInfo *)Tcl_GetHashValue(hashPtr);
    }
    else {
	Tix_MwmInfo * wmPtr;

	wmPtr = (Tix_MwmInfo*) ckalloc(sizeof(Tix_MwmInfo));
	wmPtr->interp		= interp;
	wmPtr->tkwin 	       	= tkwin;
	wmPtr->isremapping     	= 0;
	wmPtr->resetProtocol   	= 0;
	wmPtr->addedMwmMsg    	= 0;
	if (Tk_WindowId(wmPtr->tkwin) == 0) {
	    Tk_MakeWindowExist(wmPtr->tkwin);
	}
	wmPtr->mwm_hints_atom  	= Tk_InternAtom(wmPtr->tkwin, _XA_MWM_HINTS);

	Tcl_InitHashTable(&wmPtr->protocols, TCL_ONE_WORD_KEYS);

	QueryMwmHints(wmPtr);

	Tcl_SetHashValue(hashPtr, (char*)wmPtr);

	Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    StructureProc, (ClientData)wmPtr);

	return wmPtr;
    }
}

static Tix_MwmProtocol *
GetMwmProtocol(interp,  wmPtr, protocol)
    Tcl_Interp * interp;
    Tix_MwmInfo * wmPtr;
    Atom protocol;
{
    Tcl_HashEntry     * hashPtr;
    int			isNew;
    Tix_MwmProtocol   * ptPtr; 

    hashPtr = Tcl_CreateHashEntry(&wmPtr->protocols, (char*)protocol, &isNew);
    if (!isNew) {
	ptPtr = (Tix_MwmProtocol *)Tcl_GetHashValue(hashPtr);
    } else {
	ptPtr = (Tix_MwmProtocol *)ckalloc(sizeof(Tix_MwmProtocol));

	ptPtr->protocol		= protocol;
	ptPtr->name		= NULL;
	ptPtr->menuMessage	= NULL;

	Tcl_SetHashValue(hashPtr, (char*)ptPtr);
    }

    return ptPtr;
}


static int
IsMwmRunning(interp, wmPtr)
    Tcl_Interp *interp;
    Tix_MwmInfo*wmPtr;
{
    Atom motif_wm_info_atom;
    Atom actual_type;
    int	 actual_format;
    unsigned long num_items, bytes_after;
    PropMotifWmInfo *prop = 0;
    Window root;

    root = XRootWindow(Tk_Display(wmPtr->tkwin),Tk_ScreenNumber(wmPtr->tkwin));
    motif_wm_info_atom = Tk_InternAtom(wmPtr->tkwin, _XA_MOTIF_WM_INFO);

    /*
     * If mwm is running, it will store info in the _XA_MOTIF_WM_INFO
     * atom in the root window
     */
    XGetWindowProperty (Tk_Display(wmPtr->tkwin),
	root, motif_wm_info_atom, 0, (long)PROP_MOTIF_WM_INFO_ELEMENTS,
	0, motif_wm_info_atom,	&actual_type, &actual_format,
	&num_items, &bytes_after, (unsigned char **) &prop);

    if ((actual_type != motif_wm_info_atom) || (actual_format != 32) ||
	(num_items < PROP_MOTIF_WM_INFO_ELEMENTS)) {

	/*
	 * The _XA_MOTIF_WM_INFO doesn't exist for the root window.
	 * Persumably Mwm is not running.
	 */
	if (prop) {
	    XFree((char *)prop);
	}
	return(0);
    }
    else {
	/*
	 * We still need to verify that the wm_window is indeed a child of
	 * the root window.
	 */
	Window	wm_window = (Window) prop->wmWindow;
	Window	top, parent, *children;
	unsigned int num_children, i;
	int	returnVal = 0;

	if (XQueryTree(Tk_Display(wmPtr->tkwin), root, &top, &parent,
	    &children, &num_children)) {

	    for (returnVal = 0, i = 0; i < num_children; i++) {
		if (children[i] == wm_window) {
		    /*
		     * is indeed a window of this root: mwm is rinning
		     */
		    returnVal = 1;
		    break;
		}
	    }
	}

	if (prop) {
	    XFree((char *)prop);
	}
	if (children) {
	    XFree((char *)children);
	}

	return (returnVal);
    }
}
#endif
