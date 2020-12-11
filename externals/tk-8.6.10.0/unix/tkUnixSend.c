/*
 * tkUnixSend.c --
 *
 *	This file provides functions that implement the "send" command,
 *	allowing commands to be passed from interpreter to interpreter.
 *
 * Copyright (c) 1989-1994 The Regents of the University of California.
 * Copyright (c) 1994-1996 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkUnixInt.h"

/*
 * The following structure is used to keep track of the interpreters
 * registered by this process.
 */

typedef struct RegisteredInterp {
    char *name;			/* Interpreter's name (malloc-ed). */
    Tcl_Interp *interp;		/* Interpreter associated with name. NULL
				 * means that the application was unregistered
				 * or deleted while a send was in progress to
				 * it. */
    TkDisplay *dispPtr;		/* Display for the application. Needed because
				 * we may need to unregister the interpreter
				 * after its main window has been deleted. */
    struct RegisteredInterp *nextPtr;
				/* Next in list of names associated with
				 * interps in this process. NULL means end of
				 * list. */
} RegisteredInterp;

/*
 * A registry of all interpreters for a display is kept in a property
 * "InterpRegistry" on the root window of the display. It is organized as a
 * series of zero or more concatenated strings (in no particular order), each
 * of the form
 * 	window space name '\0'
 * where "window" is the hex id of the comm. window to use to talk to an
 * interpreter named "name".
 *
 * When the registry is being manipulated by an application (e.g. to add or
 * remove an entry), it is loaded into memory using a structure of the
 * following type:
 */

typedef struct NameRegistry {
    TkDisplay *dispPtr;		/* Display from which the registry was
				 * read. */
    int locked;			/* Non-zero means that the display was locked
				 * when the property was read in. */
    int modified;		/* Non-zero means that the property has been
				 * modified, so it needs to be written out
				 * when the NameRegistry is closed. */
    unsigned long propLength;	/* Length of the property, in bytes. */
    char *property;		/* The contents of the property, or NULL if
				 * none. See format description above; this is
				 * *not* terminated by the first null
				 * character. Dynamically allocated. */
    int allocedByX;		/* Non-zero means must free property with
				 * XFree; zero means use ckfree. */
} NameRegistry;

/*
 * When a result is being awaited from a sent command, one of the following
 * structures is present on a list of all outstanding sent commands. The
 * information in the structure is used to process the result when it arrives.
 * You're probably wondering how there could ever be multiple outstanding sent
 * commands. This could happen if interpreters invoke each other recursively.
 * It's unlikely, but possible.
 */

typedef struct PendingCommand {
    int serial;			/* Serial number expected in result. */
    TkDisplay *dispPtr;		/* Display being used for communication. */
    const char *target;		/* Name of interpreter command is being sent
				 * to. */
    Window commWindow;		/* Target's communication window. */
    Tcl_Interp *interp;		/* Interpreter from which the send was
				 * invoked. */
    int code;			/* Tcl return code for command will be stored
				 * here. */
    char *result;		/* String result for command (malloc'ed), or
				 * NULL. */
    char *errorInfo;		/* Information for "errorInfo" variable, or
				 * NULL (malloc'ed). */
    char *errorCode;		/* Information for "errorCode" variable, or
				 * NULL (malloc'ed). */
    int gotResponse;		/* 1 means a response has been received, 0
				 * means the command is still outstanding. */
    struct PendingCommand *nextPtr;
				/* Next in list of all outstanding commands.
				 * NULL means end of list. */
} PendingCommand;

typedef struct {
    PendingCommand *pendingCommands;
				/* List of all commands currently being waited
				 * for. */
    RegisteredInterp *interpListPtr;
				/* List of all interpreters registered in the
				 * current process. */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * The information below is used for communication between processes during
 * "send" commands. Each process keeps a private window, never even mapped,
 * with one property, "Comm". When a command is sent to an interpreter, the
 * command is appended to the comm property of the communication window
 * associated with the interp's process. Similarly, when a result is returned
 * from a sent command, it is also appended to the comm property.
 *
 * Each command and each result takes the form of ASCII text. For a command,
 * the text consists of a zero character followed by several null-terminated
 * ASCII strings. The first string consists of the single letter "c".
 * Subsequent strings have the form "option value" where the following options
 * are supported:
 *
 * -r commWindow serial
 *
 *	This option means that a response should be sent to the window whose X
 *	identifier is "commWindow" (in hex), and the response should be
 *	identified with the serial number given by "serial" (in decimal). If
 *	this option isn't specified then the send is asynchronous and no
 *	response is sent.
 *
 * -n name
 *
 *	"Name" gives the name of the application for which the command is
 *	intended. This option must be present.
 *
 * -s script
 *
 *	"Script" is the script to be executed. This option must be present.
 *
 * The options may appear in any order. The -n and -s options must be present,
 * but -r may be omitted for asynchronous RPCs. For compatibility with future
 * releases that may add new features, there may be additional options
 * present; as long as they start with a "-" character, they will be ignored.
 *
 * A result also consists of a zero character followed by several null-
 * terminated ASCII strings. The first string consists of the single letter
 * "r". Subsequent strings have the form "option value" where the following
 * options are supported:
 *
 * -s serial
 *
 *	Identifies the command for which this is the result. It is the same as
 *	the "serial" field from the -s option in the command. This option must
 *	be present.
 *
 * -c code
 *
 *	"Code" is the completion code for the script, in decimal. If the code
 *	is omitted it defaults to TCL_OK.
 *
 * -r result
 *
 *	"Result" is the result string for the script, which may be either a
 *	result or an error message. If this field is omitted then it defaults
 *	to an empty string.
 *
 * -i errorInfo
 *
 *	"ErrorInfo" gives a string with which to initialize the errorInfo
 *	variable. This option may be omitted; it is ignored unless the
 *	completion code is TCL_ERROR.
 *
 * -e errorCode
 *
 *	"ErrorCode" gives a string with with to initialize the errorCode
 *	variable. This option may be omitted; it is ignored unless the
 *	completion code is TCL_ERROR.
 *
 * Options may appear in any order, and only the -s option must be present. As
 * with commands, there may be additional options besides these; unknown
 * options are ignored.
 */

/*
 * Other miscellaneous per-process data:
 */

static struct {
    int sendSerial;		/* The serial number that was used in the last
				 * "send" command. */
    int sendDebug;		/* This can be set while debugging to do
				 * things like skip locking the server. */
} localData = {0, 0};

/*
 * Maximum size property that can be read at one time by this module:
 */

#define MAX_PROP_WORDS 100000

/*
 * Forward declarations for functions defined later in this file:
 */

static int		AppendErrorProc(ClientData clientData,
			    XErrorEvent *errorPtr);
static void		AppendPropCarefully(Display *display,
			    Window window, Atom property, char *value,
			    int length, PendingCommand *pendingPtr);
static void		DeleteProc(ClientData clientData);
static void		RegAddName(NameRegistry *regPtr,
			    const char *name, Window commWindow);
static void		RegClose(NameRegistry *regPtr);
static void		RegDeleteName(NameRegistry *regPtr, const char *name);
static Window		RegFindName(NameRegistry *regPtr, const char *name);
static NameRegistry *	RegOpen(Tcl_Interp *interp,
			    TkDisplay *dispPtr, int lock);
static void		SendEventProc(ClientData clientData, XEvent *eventPtr);
static int		SendInit(Tcl_Interp *interp, TkDisplay *dispPtr);
static Tk_RestrictProc SendRestrictProc;
static int		ServerSecure(TkDisplay *dispPtr);
static void		UpdateCommWindow(TkDisplay *dispPtr);
static int		ValidateName(TkDisplay *dispPtr, const char *name,
			    Window commWindow, int oldOK);

/*
 *----------------------------------------------------------------------
 *
 * RegOpen --
 *
 *	This function loads the name registry for a display into memory so
 *	that it can be manipulated.
 *
 * Results:
 *	The return value is a pointer to the loaded registry.
 *
 * Side effects:
 *	If "lock" is set then the server will be locked. It is the caller's
 *	responsibility to call RegClose when finished with the registry, so
 *	that we can write back the registry if needed, unlock the server if
 *	needed, and free memory.
 *
 *----------------------------------------------------------------------
 */

static NameRegistry *
RegOpen(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting
				 * (errors cause a panic so in fact no error
				 * is ever returned, but the interpreter is
				 * needed anyway). */
    TkDisplay *dispPtr,		/* Display whose name registry is to be
				 * opened. */
    int lock)			/* Non-zero means lock the window server when
				 * opening the registry, so no-one else can
				 * use the registry until we close it. */
{
    NameRegistry *regPtr;
    int result, actualFormat;
    unsigned long bytesAfter;
    Atom actualType;
    char **propertyPtr;
    Tk_ErrorHandler handler;

    if (dispPtr->commTkwin == NULL) {
	SendInit(interp, dispPtr);
    }

    handler = Tk_CreateErrorHandler(dispPtr->display, -1, -1, -1, NULL, NULL);

    regPtr = ckalloc(sizeof(NameRegistry));
    regPtr->dispPtr = dispPtr;
    regPtr->locked = 0;
    regPtr->modified = 0;
    regPtr->allocedByX = 1;
    propertyPtr = &regPtr->property;

    if (lock && !localData.sendDebug) {
	XGrabServer(dispPtr->display);
	regPtr->locked = 1;
    }

    /*
     * Read the registry property.
     */

    result = XGetWindowProperty(dispPtr->display,
	    RootWindow(dispPtr->display, 0),
	    dispPtr->registryProperty, 0, MAX_PROP_WORDS,
	    False, XA_STRING, &actualType, &actualFormat,
	    &regPtr->propLength, &bytesAfter,
	    (unsigned char **) propertyPtr);

    if (actualType == None) {
	regPtr->propLength = 0;
	regPtr->property = NULL;
    } else if ((result != Success) || (actualFormat != 8)
	    || (actualType != XA_STRING)) {
	/*
	 * The property is improperly formed; delete it.
	 */

	if (regPtr->property != NULL) {
	    XFree(regPtr->property);
	    regPtr->propLength = 0;
	    regPtr->property = NULL;
	}
	XDeleteProperty(dispPtr->display,
		RootWindow(dispPtr->display, 0),
		dispPtr->registryProperty);
        XSync(dispPtr->display, False);
    }

    Tk_DeleteErrorHandler(handler);

    /*
     * Xlib placed an extra null byte after the end of the property, just to
     * make sure that it is always NULL-terminated. Be sure to include this
     * byte in our count if it's needed to ensure null termination (note: as
     * of 8/95 I'm no longer sure why this code is needed; seems like it
     * shouldn't be).
     */

    if ((regPtr->propLength > 0)
	    && (regPtr->property[regPtr->propLength-1] != 0)) {
	regPtr->propLength++;
    }
    return regPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * RegFindName --
 *
 *	Given an open name registry, this function finds an entry with a given
 *	name, if there is one, and returns information about that entry.
 *
 * Results:
 *	The return value is the X identifier for the comm window for the
 *	application named "name", or None if there is no such entry in the
 *	registry.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Window
RegFindName(
    NameRegistry *regPtr,	/* Pointer to a registry opened with a
				 * previous call to RegOpen. */
    const char *name)		/* Name of an application. */
{
    char *p;

    for (p=regPtr->property ; p-regPtr->property<(int)regPtr->propLength ;) {
	char *entry = p;

	while ((*p != 0) && (!isspace(UCHAR(*p)))) {
	    p++;
	}
	if ((*p != 0) && (strcmp(name, p+1) == 0)) {
	    unsigned id;

	    if (sscanf(entry, "%x", &id) == 1) {
		/*
		 * Must cast from an unsigned int to a Window in case we are
		 * on a 64-bit architecture.
		 */

		return (Window) id;
	    }
	}
	while (*p != 0) {
	    p++;
	}
	p++;
    }
    return None;
}

/*
 *----------------------------------------------------------------------
 *
 * RegDeleteName --
 *
 *	This function deletes the entry for a given name from an open
 *	registry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there used to be an entry named "name" in the registry, then it is
 *	deleted and the registry is marked as modified so it will be written
 *	back when closed.
 *
 *----------------------------------------------------------------------
 */

static void
RegDeleteName(
    NameRegistry *regPtr,	/* Pointer to a registry opened with a
				 * previous call to RegOpen. */
    const char *name)		/* Name of an application. */
{
    char *p;

    for (p=regPtr->property ; p-regPtr->property<(int)regPtr->propLength ;) {
	char *entry = p, *entryName;

	while ((*p != 0) && (!isspace(UCHAR(*p)))) {
	    p++;
	}
	if (*p != 0) {
	    p++;
	}
	entryName = p;
	while (*p != 0) {
	    p++;
	}
	p++;
	if (strcmp(name, entryName) == 0) {
	    int count;

	    /*
	     * Found the matching entry. Copy everything after it down on top
	     * of it.
	     */

	    count = regPtr->propLength - (p - regPtr->property);
	    if (count > 0) {
		char *src, *dst;

		for (src=p , dst=entry ; count>0 ; src++, dst++, count--) {
		    *dst = *src;
		}
	    }
	    regPtr->propLength -= p - entry;
	    regPtr->modified = 1;
	    return;
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RegAddName --
 *
 *	Add a new entry to an open registry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The open registry is expanded; it is marked as modified so that it
 *	will be written back when closed.
 *
 *----------------------------------------------------------------------
 */

static void
RegAddName(
    NameRegistry *regPtr,	/* Pointer to a registry opened with a
				 * previous call to RegOpen. */
    const char *name,		/* Name of an application. The caller must
				 * ensure that this name isn't already
				 * registered. */
    Window commWindow)		/* X identifier for comm. window of
				 * application. */
{
    char id[30], *newProp;
    int idLength, newBytes;

    sprintf(id, "%x ", (unsigned) commWindow);
    idLength = strlen(id);
    newBytes = idLength + strlen(name) + 1;
    newProp = ckalloc(regPtr->propLength + newBytes);
    strcpy(newProp, id);
    strcpy(newProp+idLength, name);
    if (regPtr->property != NULL) {
	memcpy(newProp + newBytes, regPtr->property, regPtr->propLength);
	if (regPtr->allocedByX) {
	    XFree(regPtr->property);
	} else {
	    ckfree(regPtr->property);
	}
    }
    regPtr->modified = 1;
    regPtr->propLength += newBytes;
    regPtr->property = newProp;
    regPtr->allocedByX = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * RegClose --
 *
 *	This function is called to end a series of operations on a name
 *	registry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The registry is written back if it has been modified, and the X server
 *	is unlocked if it was locked. Memory for the registry is freed, so the
 *	caller should never use regPtr again.
 *
 *----------------------------------------------------------------------
 */

static void
RegClose(
    NameRegistry *regPtr)	/* Pointer to a registry opened with a
				 * previous call to RegOpen. */
{
    Tk_ErrorHandler handler;

    handler = Tk_CreateErrorHandler(regPtr->dispPtr->display, -1, -1, -1,
            NULL, NULL);

    if (regPtr->modified) {
	if (!regPtr->locked && !localData.sendDebug) {
	    Tcl_Panic("The name registry was modified without being locked!");
	}
	XChangeProperty(regPtr->dispPtr->display,
		RootWindow(regPtr->dispPtr->display, 0),
		regPtr->dispPtr->registryProperty, XA_STRING, 8,
		PropModeReplace, (unsigned char *) regPtr->property,
		(int) regPtr->propLength);
    }

    if (regPtr->locked) {
	XUngrabServer(regPtr->dispPtr->display);
    }

    /*
     * After ungrabbing the server, it's important to flush the output
     * immediately so that the server sees the ungrab command. Otherwise we
     * might do something else that needs to communicate with the server (such
     * as invoking a subprocess that needs to do I/O to the screen); if the
     * ungrab command is still sitting in our output buffer, we could
     * deadlock.
     */

    XFlush(regPtr->dispPtr->display);

    Tk_DeleteErrorHandler(handler);

    if (regPtr->property != NULL) {
	if (regPtr->allocedByX) {
	    XFree(regPtr->property);
	} else {
	    ckfree(regPtr->property);
	}
    }
    ckfree(regPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ValidateName --
 *
 *	This function checks to see if an entry in the registry is still
 *	valid.
 *
 * Results:
 *	The return value is 1 if the given commWindow exists and its name is
 *	"name". Otherwise 0 is returned.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ValidateName(
    TkDisplay *dispPtr,		/* Display for which to perform the
				 * validation. */
    const char *name,		/* The name of an application. */
    Window commWindow,		/* X identifier for the application's comm.
				 * window. */
    int oldOK)			/* Non-zero means that we should consider an
				 * application to be valid even if it looks
				 * like an old-style (pre-4.0) one; 0 means
				 * consider these invalid. */
{
    int result, actualFormat, argc, i;
    unsigned long length, bytesAfter;
    Atom actualType;
    char *property, **propertyPtr = &property;
    Tk_ErrorHandler handler;
    const char **argv;

    property = NULL;

    /*
     * Ignore X errors when reading the property (e.g., the window might not
     * exist). If an error occurs, result will be some value other than
     * Success.
     */

    handler = Tk_CreateErrorHandler(dispPtr->display, -1, -1, -1, NULL, NULL);
    result = XGetWindowProperty(dispPtr->display, commWindow,
	    dispPtr->appNameProperty, 0, MAX_PROP_WORDS,
	    False, XA_STRING, &actualType, &actualFormat,
	    &length, &bytesAfter, (unsigned char **) propertyPtr);

    if ((result == Success) && (actualType == None)) {
	XWindowAttributes atts;

	/*
	 * The comm. window exists but the property we're looking for doesn't
	 * exist. This probably means that the application comes from an older
	 * version of Tk (< 4.0) that didn't set the property; if this is the
	 * case, then assume for compatibility's sake that everything's OK.
	 * However, it's also possible that some random application has
	 * re-used the window id for something totally unrelated. Check a few
	 * characteristics of the window, such as its dimensions and mapped
	 * state, to be sure that it still "smells" like a commWindow.
	 */

	if (!oldOK
		|| !XGetWindowAttributes(dispPtr->display, commWindow, &atts)
		|| (atts.width != 1) || (atts.height != 1)
		|| (atts.map_state != IsUnmapped)) {
	    result = 0;
	} else {
	    result = 1;
	}
    } else if ((result == Success) && (actualFormat == 8)
	    && (actualType == XA_STRING)) {
	result = 0;
	if (Tcl_SplitList(NULL, property, &argc, &argv) == TCL_OK) {
	    for (i = 0; i < argc; i++) {
		if (strcmp(argv[i], name) == 0) {
		    result = 1;
		    break;
		}
	    }
	    ckfree(argv);
	}
    } else {
	result = 0;
    }
    Tk_DeleteErrorHandler(handler);
    if (property != NULL) {
	XFree(property);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * ServerSecure --
 *
 *	Check whether a server is secure enough for us to trust Tcl scripts
 *	arriving via that server.
 *
 * Results:
 *	The return value is 1 if the server is secure, which means that
 *	host-style authentication is turned on but there are no hosts in the
 *	enabled list. This means that some other form of authorization
 *	(presumably more secure, such as xauth) is in use.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ServerSecure(
    TkDisplay *dispPtr)		/* Display to check. */
{
#ifdef TK_NO_SECURITY
    return 1;
#else
    XHostAddress *addrPtr;
    int numHosts, secure;
    Bool enabled;

    addrPtr = XListHosts(dispPtr->display, &numHosts, &enabled);
    if (!enabled) {
    insecure:
	secure = 0;
    } else if (numHosts == 0) {
	secure = 1;
    } else {
	/*
	 * Recent versions of X11 have the extra feature of allowing more
	 * sophisticated authorization checks to be performed than the dozy
	 * old ones that used to plague xhost usage. However, not all deployed
	 * versions of Xlib know how to deal with this feature, so this code
	 * is conditional on having the right #def in place. [Bug 1909931]
	 *
	 * Note that at this point we know that there's at least one entry in
	 * the list returned by XListHosts. However there may be multiple
	 * entries; as long as each is one of either 'SI:localhost:*' or
	 * 'SI:localgroup:*' then we will claim to be secure enough.
	 */

#ifdef FamilyServerInterpreted
	XServerInterpretedAddress *siPtr;
	int i;

	for (i=0 ; i<numHosts ; i++) {
	    if (addrPtr[i].family != FamilyServerInterpreted) {
		/*
		 * We don't understand what the X server is letting in, so we
		 * err on the side of safety.
		 */

		goto insecure;
	    }
	    siPtr = (XServerInterpretedAddress *) addrPtr[0].address;

	    /*
	     * We don't check the username or group here. This is because it's
	     * officially non-portable and we are just making sure there
	     * aren't silly misconfigurations. (Apparently 'root' is not a
	     * very good choice, but we still don't put any effort in to spot
	     * that.) However we do check to see that the constraints are
	     * imposed against the connecting user and/or group.
	     */

	    if (       !(siPtr->typelength == 9 /* ==strlen("localuser") */
			&& !memcmp(siPtr->type, "localuser", 9))
		    && !(siPtr->typelength == 10 /* ==strlen("localgroup") */
			&& !memcmp(siPtr->type, "localgroup", 10))) {
		/*
		 * The other defined types of server-interpreted controls
		 * involve particular hosts. These are still insecure for the
		 * same reasons that classic xhost access is insecure; there's
		 * just no way to be sure that the users on those systems are
		 * the ones who should be allowed to connect to this display.
		 */

		goto insecure;
	    }
	}
	secure = 1;
#else
	/*
	 * We don't understand what the X server is letting in, so we err on
	 * the side of safety.
	 */

	secure = 0;
#endif /* FamilyServerInterpreted */
    }
    if (addrPtr != NULL) {
	XFree((char *) addrPtr);
    }
    return secure;
#endif /* TK_NO_SECURITY */
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_SetAppName --
 *
 *	This function is called to associate an ASCII name with a Tk
 *	application. If the application has already been named, the name
 *	replaces the old one.
 *
 * Results:
 *	The return value is the name actually given to the application. This
 *	will normally be the same as name, but if name was already in use for
 *	an application then a name of the form "name #2" will be chosen, with
 *	a high enough number to make the name unique.
 *
 * Side effects:
 *	Registration info is saved, thereby allowing the "send" command to be
 *	used later to invoke commands in the application. In addition, the
 *	"send" command is created in the application's interpreter. The
 *	registration will be removed automatically if the interpreter is
 *	deleted or the "send" command is removed.
 *
 *----------------------------------------------------------------------
 */

const char *
Tk_SetAppName(
    Tk_Window tkwin,		/* Token for any window in the application to
				 * be named: it is just used to identify the
				 * application and the display. */
    const char *name)		/* The name that will be used to refer to the
				 * interpreter in later "send" commands. Must
				 * be globally unique. */
{
    RegisteredInterp *riPtr, *riPtr2;
    Window w;
    TkWindow *winPtr = (TkWindow *) tkwin;
    TkDisplay *dispPtr = winPtr->dispPtr;
    NameRegistry *regPtr;
    Tcl_Interp *interp;
    const char *actualName;
    Tcl_DString dString;
    int offset, i;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    interp = winPtr->mainPtr->interp;
    if (dispPtr->commTkwin == NULL) {
	SendInit(interp, winPtr->dispPtr);
    }

    /*
     * See if the application is already registered; if so, remove its current
     * name from the registry.
     */

    regPtr = RegOpen(interp, winPtr->dispPtr, 1);
    for (riPtr = tsdPtr->interpListPtr; ; riPtr = riPtr->nextPtr) {
	if (riPtr == NULL) {
	    /*
	     * This interpreter isn't currently registered; create the data
	     * structure that will be used to register it locally, plus add
	     * the "send" command to the interpreter.
	     */

	    riPtr = ckalloc(sizeof(RegisteredInterp));
	    riPtr->interp = interp;
	    riPtr->dispPtr = winPtr->dispPtr;
	    riPtr->nextPtr = tsdPtr->interpListPtr;
	    tsdPtr->interpListPtr = riPtr;
	    riPtr->name = NULL;
	    Tcl_CreateObjCommand(interp, "send", Tk_SendObjCmd, riPtr, DeleteProc);
	    if (Tcl_IsSafe(interp)) {
		Tcl_HideCommand(interp, "send", "send");
	    }
	    break;
	}
	if (riPtr->interp == interp) {
	    /*
	     * The interpreter is currently registered; remove it from the
	     * name registry.
	     */

	    if (riPtr->name) {
		RegDeleteName(regPtr, riPtr->name);
		ckfree(riPtr->name);
	    }
	    break;
	}
    }

    /*
     * Pick a name to use for the application. Use "name" if it's not already
     * in use. Otherwise add a suffix such as " #2", trying larger and larger
     * numbers until we eventually find one that is unique.
     */

    actualName = name;
    offset = 0;				/* Needed only to avoid "used before
					 * set" compiler warnings. */
    for (i = 1; ; i++) {
	if (i > 1) {
	    if (i == 2) {
		Tcl_DStringInit(&dString);
		Tcl_DStringAppend(&dString, name, -1);
		Tcl_DStringAppend(&dString, " #", 2);
		offset = Tcl_DStringLength(&dString);
		Tcl_DStringSetLength(&dString, offset+TCL_INTEGER_SPACE);
		actualName = Tcl_DStringValue(&dString);
	    }
	    sprintf(Tcl_DStringValue(&dString) + offset, "%d", i);
	}
	w = RegFindName(regPtr, actualName);
	if (w == None) {
	    break;
	}

	/*
	 * The name appears to be in use already, but double-check to be sure
	 * (perhaps the application died without removing its name from the
	 * registry?).
	 */

	if (w == Tk_WindowId(dispPtr->commTkwin)) {
	    for (riPtr2 = tsdPtr->interpListPtr; riPtr2 != NULL;
		    riPtr2 = riPtr2->nextPtr) {
		if ((riPtr2->interp != interp) &&
			(strcmp(riPtr2->name, actualName) == 0)) {
		    goto nextSuffix;
		}
	    }
	    RegDeleteName(regPtr, actualName);
	    break;
	} else if (!ValidateName(winPtr->dispPtr, actualName, w, 1)) {
	    RegDeleteName(regPtr, actualName);
	    break;
	}
    nextSuffix:
	continue;
    }

    /*
     * We've now got a name to use. Store it in the name registry and in the
     * local entry for this application, plus put it in a property on the
     * commWindow.
     */

    RegAddName(regPtr, actualName, Tk_WindowId(dispPtr->commTkwin));
    RegClose(regPtr);
    riPtr->name = ckalloc(strlen(actualName) + 1);
    strcpy(riPtr->name, actualName);
    if (actualName != name) {
	Tcl_DStringFree(&dString);
    }
    UpdateCommWindow(dispPtr);

    return riPtr->name;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_SendObjCmd --
 *
 *	This function is invoked to process the "send" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_SendObjCmd(
    ClientData clientData,	/* Information about sender (only dispPtr
				 * field is used). */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    enum {
	SEND_ASYNC, SEND_DISPLAYOF, SEND_LAST
    };
    static const char *const sendOptions[] = {
	"-async",   "-displayof",   "--",  NULL
    };
    TkWindow *winPtr;
    Window commWindow;
    PendingCommand pending;
    register RegisteredInterp *riPtr;
    const char *destName;
    int result, index, async, i, firstArg;
    Tk_RestrictProc *prevProc;
    ClientData prevArg;
    TkDisplay *dispPtr;
    Tcl_Time timeout;
    NameRegistry *regPtr;
    Tcl_DString request;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
    Tcl_Interp *localInterp;	/* Used when the interpreter to send the
				 * command to is within the same process. */

    /*
     * Process options, if any.
     */

    async = 0;
    winPtr = (TkWindow *) Tk_MainWindow(interp);
    if (winPtr == NULL) {
	return TCL_ERROR;
    }
    for (i = 1; i < objc; i++) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], sendOptions,
		sizeof(char *), "option", 0, &index) != TCL_OK) {
	    break;
	}
	if (index == SEND_ASYNC) {
	    ++async;
	} else if (index == SEND_DISPLAYOF) {
	    winPtr = (TkWindow *) Tk_NameToWindow(interp, Tcl_GetString(objv[++i]),
		    (Tk_Window) winPtr);
	    if (winPtr == NULL) {
		return TCL_ERROR;
	    }
	} else if (index == SEND_LAST) {
	    i++;
	    break;
	}
    }

    if (objc < (i+2)) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"?-option value ...? interpName arg ?arg ...?");
	return TCL_ERROR;
    }
    destName = Tcl_GetString(objv[i]);
    firstArg = i+1;

    dispPtr = winPtr->dispPtr;
    if (dispPtr->commTkwin == NULL) {
	SendInit(interp, winPtr->dispPtr);
    }

    /*
     * See if the target interpreter is local. If so, execute the command
     * directly without going through the X server. The only tricky thing is
     * passing the result from the target interpreter to the invoking
     * interpreter. Watch out: they could be the same!
     */

    for (riPtr = tsdPtr->interpListPtr; riPtr != NULL;
	    riPtr = riPtr->nextPtr) {
	if ((riPtr->dispPtr != dispPtr)
		|| (strcmp(riPtr->name, destName) != 0)) {
	    continue;
	}
	Tcl_Preserve(riPtr);
	localInterp = riPtr->interp;
	Tcl_Preserve(localInterp);
	if (firstArg == (objc-1)) {
	    result = Tcl_EvalEx(localInterp, Tcl_GetString(objv[firstArg]), -1, TCL_EVAL_GLOBAL);
	} else {
	    Tcl_DStringInit(&request);
	    Tcl_DStringAppend(&request, Tcl_GetString(objv[firstArg]), -1);
	    for (i = firstArg+1; i < objc; i++) {
		Tcl_DStringAppend(&request, " ", 1);
		Tcl_DStringAppend(&request, Tcl_GetString(objv[i]), -1);
	    }
	    result = Tcl_EvalEx(localInterp, Tcl_DStringValue(&request), -1, TCL_EVAL_GLOBAL);
	    Tcl_DStringFree(&request);
	}
	if (interp != localInterp) {
	    if (result == TCL_ERROR) {
		Tcl_Obj *errorObjPtr;

		/*
		 * An error occurred, so transfer error information from the
		 * destination interpreter back to our interpreter. Must clear
		 * interp's result before calling Tcl_AddErrorInfo, since
		 * Tcl_AddErrorInfo will store the interp's result in
		 * errorInfo before appending riPtr's $errorInfo; we've
		 * already got everything we need in riPtr's $errorInfo.
		 */

		Tcl_ResetResult(interp);
		Tcl_AddErrorInfo(interp, Tcl_GetVar2(localInterp,
			"errorInfo", NULL, TCL_GLOBAL_ONLY));
		errorObjPtr = Tcl_GetVar2Ex(localInterp, "errorCode", NULL,
			TCL_GLOBAL_ONLY);
		Tcl_SetObjErrorCode(interp, errorObjPtr);
	    }
	    Tcl_SetObjResult(interp, Tcl_GetObjResult(localInterp));
	    Tcl_ResetResult(localInterp);
	}
	Tcl_Release(riPtr);
	Tcl_Release(localInterp);
	return result;
    }

    /*
     * Bind the interpreter name to a communication window.
     */

    regPtr = RegOpen(interp, winPtr->dispPtr, 0);
    commWindow = RegFindName(regPtr, destName);
    RegClose(regPtr);
    if (commWindow == None) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"no application named \"%s\"", destName));
	Tcl_SetErrorCode(interp, "TK", "LOOKUP", "APPLICATION", destName,
		NULL);
	return TCL_ERROR;
    }

    /*
     * Send the command to the target interpreter by appending it to the comm
     * window in the communication window.
     */

    localData.sendSerial++;
    Tcl_DStringInit(&request);
    Tcl_DStringAppend(&request, "\0c\0-n ", 6);
    Tcl_DStringAppend(&request, destName, -1);
    if (!async) {
	char buffer[TCL_INTEGER_SPACE * 2];

	sprintf(buffer, "%x %d",
		(unsigned) Tk_WindowId(dispPtr->commTkwin),
		localData.sendSerial);
	Tcl_DStringAppend(&request, "\0-r ", 4);
	Tcl_DStringAppend(&request, buffer, -1);
    }
    Tcl_DStringAppend(&request, "\0-s ", 4);
    Tcl_DStringAppend(&request, Tcl_GetString(objv[firstArg]), -1);
    for (i = firstArg+1; i < objc; i++) {
	Tcl_DStringAppend(&request, " ", 1);
	Tcl_DStringAppend(&request, Tcl_GetString(objv[i]), -1);
    }

    if (!async) {
	/*
	 * Register the fact that we're waiting for a command to complete
	 * (this is needed by SendEventProc and by AppendErrorProc to pass
	 * back the command's results). Set up a timeout handler so that
	 * we can check during long sends to make sure that the destination
	 * application is still alive.
	 *
	 * We prepare the pending struct here in order to catch potential
	 * early X errors from AppendPropCarefully() due to XSync().
	 */

	pending.serial = localData.sendSerial;
	pending.dispPtr = dispPtr;
	pending.target = destName;
	pending.commWindow = commWindow;
	pending.interp = interp;
	pending.result = NULL;
	pending.errorInfo = NULL;
	pending.errorCode = NULL;
	pending.gotResponse = 0;
	pending.nextPtr = tsdPtr->pendingCommands;
	tsdPtr->pendingCommands = &pending;
    }
    (void) AppendPropCarefully(dispPtr->display, commWindow,
	    dispPtr->commProperty, Tcl_DStringValue(&request),
	    Tcl_DStringLength(&request) + 1, (async ? NULL : &pending));
    Tcl_DStringFree(&request);
    if (async) {
	/*
	 * This is an asynchronous send: return immediately without waiting
	 * for a response.
	 */

	return TCL_OK;
    }

    /*
     * Enter a loop processing X events until the result comes in or the
     * target is declared to be dead. While waiting for a result, look only at
     * send-related events so that the send is synchronous with respect to
     * other events in the application.
     */

    prevProc = Tk_RestrictEvents(SendRestrictProc, NULL, &prevArg);
    Tcl_GetTime(&timeout);
    timeout.sec += 2;
    while (!pending.gotResponse) {
	if (!TkUnixDoOneXEvent(&timeout)) {
	    /*
	     * An unusually long amount of time has elapsed during the
	     * processing of a sent command. Check to make sure that the
	     * target application still exists. If it does, reset the timeout.
	     */

	    if (!ValidateName(pending.dispPtr, pending.target,
		    pending.commWindow, 0)) {
		const char *msg;

		if (ValidateName(pending.dispPtr, pending.target,
			pending.commWindow, 1)) {
		    msg = "target application died or uses a Tk version before 4.0";
		} else {
		    msg = "target application died";
		}
		pending.code = TCL_ERROR;
		pending.result = ckalloc(strlen(msg) + 1);
		strcpy(pending.result, msg);
		pending.gotResponse = 1;
	    } else {
		Tcl_GetTime(&timeout);
		timeout.sec += 2;
	    }
	}
    }
    Tk_RestrictEvents(prevProc, prevArg, &prevArg);

    /*
     * Unregister the information about the pending command and return the
     * result.
     */

    if (tsdPtr->pendingCommands != &pending) {
	Tcl_Panic("Tk_SendCmd: corrupted send stack");
    }
    tsdPtr->pendingCommands = pending.nextPtr;
    if (pending.errorInfo != NULL) {
	/*
	 * Special trick: must clear the interp's result before calling
	 * Tcl_AddErrorInfo, since Tcl_AddErrorInfo will store the interp's
	 * result in errorInfo before appending pending.errorInfo; we've
	 * already got everything we need in pending.errorInfo.
	 */

	Tcl_ResetResult(interp);
	Tcl_AddErrorInfo(interp, pending.errorInfo);
	ckfree(pending.errorInfo);
    }
    if (pending.errorCode != NULL) {
	Tcl_SetObjErrorCode(interp, Tcl_NewStringObj(pending.errorCode, -1));
	ckfree(pending.errorCode);
    }
    Tcl_SetObjResult(interp, Tcl_NewStringObj(pending.result, -1));
    ckfree(pending.result);
    return pending.code;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetInterpNames --
 *
 *	This function is invoked to fetch a list of all the interpreter names
 *	currently registered for the display of a particular window.
 *
 * Results:
 *	A standard Tcl return value. The interp's result will be set to hold a
 *	list of all the interpreter names defined for tkwin's display. If an
 *	error occurs, then TCL_ERROR is returned and the interp's result will
 *	hold an error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkGetInterpNames(
    Tcl_Interp *interp,		/* Interpreter for returning a result. */
    Tk_Window tkwin)		/* Window whose display is to be used for the
				 * lookup. */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    NameRegistry *regPtr;
    Tcl_Obj *resultObj = Tcl_NewObj();
    char *p;

    /*
     * Read the registry property, then scan through all of its entries.
     * Validate each entry to be sure that its application still exists.
     */

    regPtr = RegOpen(interp, winPtr->dispPtr, 1);
    for (p=regPtr->property ; p-regPtr->property<(int)regPtr->propLength ;) {
	char *entry = p, *entryName;
	Window commWindow;
	unsigned id;

	if (sscanf(p, "%x", (unsigned *) &id) != 1) {
	    commWindow = None;
	} else {
	    commWindow = id;
	}
	while ((*p != 0) && (!isspace(UCHAR(*p)))) {
	    p++;
	}
	if (*p != 0) {
	    p++;
	}
	entryName = p;
	while (*p != 0) {
	    p++;
	}
	p++;
	if (ValidateName(winPtr->dispPtr, entryName, commWindow, 1)) {
	    /*
	     * The application still exists; add its name to the result.
	     */

	    Tcl_ListObjAppendElement(NULL, resultObj,
		    Tcl_NewStringObj(entryName, -1));
	} else {
	    int count;

	    /*
	     * This name is bogus (perhaps the application died without
	     * cleaning up its entry in the registry?). Delete the name.
	     */

	    count = regPtr->propLength - (p - regPtr->property);
	    if (count > 0) {
		char *src, *dst;

		for (src = p, dst = entry; count > 0; src++, dst++, count--) {
		    *dst = *src;
		}
	    }
	    regPtr->propLength -= p - entry;
	    regPtr->modified = 1;
	    p = entry;
	}
    }
    RegClose(regPtr);
    Tcl_SetObjResult(interp, resultObj);
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * TkSendCleanup --
 *
 *	This function is called to free resources used by the communication
 *	channels for sending commands and receiving results.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees various data structures and windows.
 *
 *--------------------------------------------------------------
 */

void
TkSendCleanup(
    TkDisplay *dispPtr)
{
    if (dispPtr->commTkwin != NULL) {
	Tk_DeleteEventHandler(dispPtr->commTkwin, PropertyChangeMask,
		SendEventProc, dispPtr);
	Tk_DestroyWindow(dispPtr->commTkwin);
	Tcl_Release(dispPtr->commTkwin);
	dispPtr->commTkwin = NULL;
    }
}

/*
 *--------------------------------------------------------------
 *
 * SendInit --
 *
 *	This function is called to initialize the communication channels for
 *	sending commands and receiving results.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets up various data structures and windows.
 *
 *--------------------------------------------------------------
 */

static int
SendInit(
    Tcl_Interp *interp,		/* Interpreter to use for error reporting (no
				 * errors are ever returned, but the
				 * interpreter is needed anyway). */
    TkDisplay *dispPtr)		/* Display to initialize. */
{
    XSetWindowAttributes atts;

    /*
     * Create the window used for communication, and set up an event handler
     * for it.
     */

    dispPtr->commTkwin = (Tk_Window) TkAllocWindow(dispPtr,
    	DefaultScreen(dispPtr->display), NULL);
    Tcl_Preserve(dispPtr->commTkwin);
    ((TkWindow *) dispPtr->commTkwin)->flags |=
	    TK_TOP_HIERARCHY|TK_TOP_LEVEL|TK_HAS_WRAPPER|TK_WIN_MANAGED;
    TkWmNewWindow((TkWindow *) dispPtr->commTkwin);
    atts.override_redirect = True;
    Tk_ChangeWindowAttributes(dispPtr->commTkwin,
	    CWOverrideRedirect, &atts);
    Tk_CreateEventHandler(dispPtr->commTkwin, PropertyChangeMask,
	    SendEventProc, dispPtr);
    Tk_MakeWindowExist(dispPtr->commTkwin);

    /*
     * Get atoms used as property names.
     */

    dispPtr->commProperty = Tk_InternAtom(dispPtr->commTkwin, "Comm");
    dispPtr->registryProperty = Tk_InternAtom(dispPtr->commTkwin,
	    "InterpRegistry");
    dispPtr->appNameProperty = Tk_InternAtom(dispPtr->commTkwin,
	    "TK_APPLICATION");

    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * SendEventProc --
 *
 *	This function is invoked automatically by the toolkit event manager
 *	when a property changes on the communication window. This function
 *	reads the property and handles command requests and responses.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If there are command requests in the property, they are executed. If
 *	there are responses in the property, their information is saved for
 *	the (ostensibly waiting) "send" commands. The property is deleted.
 *
 *--------------------------------------------------------------
 */

static void
SendEventProc(
    ClientData clientData,	/* Display information. */
    XEvent *eventPtr)		/* Information about event. */
{
    TkDisplay *dispPtr = clientData;
    char *propInfo, **propInfoPtr = &propInfo;
    const char *p;
    int result, actualFormat;
    unsigned long numItems, bytesAfter;
    Atom actualType;
    Tcl_Interp *remoteInterp;	/* Interp in which to execute the command. */
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if ((eventPtr->xproperty.atom != dispPtr->commProperty)
	    || (eventPtr->xproperty.state != PropertyNewValue)) {
	return;
    }

    /*
     * Read the comm property and delete it.
     */

    propInfo = NULL;
    result = XGetWindowProperty(dispPtr->display,
	    Tk_WindowId(dispPtr->commTkwin), dispPtr->commProperty, 0,
	    MAX_PROP_WORDS, True, XA_STRING, &actualType, &actualFormat,
	    &numItems, &bytesAfter, (unsigned char **) propInfoPtr);

    /*
     * If the property doesn't exist or is improperly formed then ignore it.
     */

    if ((result != Success) || (actualType != XA_STRING)
	    || (actualFormat != 8)) {
	if (propInfo != NULL) {
	    XFree(propInfo);
	}
	return;
    }

    /*
     * Several commands and results could arrive in the property at one time;
     * each iteration through the outer loop handles a single command or
     * result.
     */

    for (p = propInfo; (p-propInfo) < (int) numItems; ) {
	/*
	 * Ignore leading NULLs; each command or result starts with a NULL so
	 * that no matter how badly formed a preceding command is, we'll be
	 * able to tell that a new command/result is starting.
	 */

	if (*p == 0) {
	    p++;
	    continue;
	}

	if ((*p == 'c') && (p[1] == 0)) {
	    Window commWindow;
	    const char *interpName, *script, *serial;
	    char *end;
	    Tcl_DString reply;
	    RegisteredInterp *riPtr;

	    /*
	     *----------------------------------------------------------
	     * This is an incoming command from some other application.
	     * Iterate over all of its options. Stop when we reach the end of
	     * the property or something that doesn't look like an option.
	     *----------------------------------------------------------
	     */

	    p += 2;
	    interpName = NULL;
	    commWindow = None;
	    serial = "";
	    script = NULL;
	    while (((p-propInfo) < (int) numItems) && (*p == '-')) {
		switch (p[1]) {
		case 'r':
		    commWindow = (Window) strtoul(p+2, &end, 16);
		    if ((end == p+2) || (*end != ' ')) {
			commWindow = None;
		    } else {
			p = serial = end+1;
		    }
		    break;
		case 'n':
		    if (p[2] == ' ') {
			interpName = p+3;
		    }
		    break;
		case 's':
		    if (p[2] == ' ') {
			script = p+3;
		    }
		    break;
		}
		while (*p != 0) {
		    p++;
		}
		p++;
	    }

	    if ((script == NULL) || (interpName == NULL)) {
		continue;
	    }

	    /*
	     * Initialize the result property, so that we're ready at any time
	     * if we need to return an error.
	     */

	    if (commWindow != None) {
		Tcl_DStringInit(&reply);
		Tcl_DStringAppend(&reply, "\0r\0-s ", 6);
		Tcl_DStringAppend(&reply, serial, -1);
		Tcl_DStringAppend(&reply, "\0-r ", 4);
	    }

	    if (!ServerSecure(dispPtr)) {
		if (commWindow != None) {
		    Tcl_DStringAppend(&reply,
			    "X server insecure (must use xauth-style "
			    "authorization); command ignored", -1);
		}
		result = TCL_ERROR;
		goto returnResult;
	    }

	    /*
	     * Locate the application, then execute the script.
	     */

	    for (riPtr = tsdPtr->interpListPtr; ; riPtr = riPtr->nextPtr) {
		if (riPtr == NULL) {
		    if (commWindow != None) {
			Tcl_DStringAppend(&reply,
				"receiver never heard of interpreter \"", -1);
			Tcl_DStringAppend(&reply, interpName, -1);
			Tcl_DStringAppend(&reply, "\"", 1);
		    }
		    result = TCL_ERROR;
		    goto returnResult;
		}
		if (strcmp(riPtr->name, interpName) == 0) {
		    break;
		}
	    }
	    Tcl_Preserve(riPtr);

	    /*
	     * We must protect the interpreter because the script may enter
	     * another event loop, which might call Tcl_DeleteInterp.
	     */

	    remoteInterp = riPtr->interp;
	    Tcl_Preserve(remoteInterp);

	    result = Tcl_EvalEx(remoteInterp, script, -1, TCL_EVAL_GLOBAL);

	    /*
	     * The call to Tcl_Release may have released the interpreter which
	     * will cause the "send" command for that interpreter to be
	     * deleted. The command deletion callback will set the
	     * riPtr->interp field to NULL, hence the check below for NULL.
	     */

	    if (commWindow != None) {
		Tcl_DStringAppend(&reply, Tcl_GetString(Tcl_GetObjResult(remoteInterp)),
			-1);
		if (result == TCL_ERROR) {
		    const char *varValue;

		    varValue = Tcl_GetVar2(remoteInterp, "errorInfo",
			    NULL, TCL_GLOBAL_ONLY);
		    if (varValue != NULL) {
			Tcl_DStringAppend(&reply, "\0-i ", 4);
			Tcl_DStringAppend(&reply, varValue, -1);
		    }
		    varValue = Tcl_GetVar2(remoteInterp, "errorCode",
			    NULL, TCL_GLOBAL_ONLY);
		    if (varValue != NULL) {
			Tcl_DStringAppend(&reply, "\0-e ", 4);
			Tcl_DStringAppend(&reply, varValue, -1);
		    }
		}
	    }
	    Tcl_Release(remoteInterp);
	    Tcl_Release(riPtr);

	    /*
	     * Return the result to the sender if a commWindow was specified
	     * (if none was specified then this is an asynchronous call).
	     * Right now reply has everything but the completion code, but it
	     * needs the NULL to terminate the current option.
	     */

	returnResult:
	    if (commWindow != None) {
		if (result != TCL_OK) {
		    char buffer[TCL_INTEGER_SPACE];

		    sprintf(buffer, "%d", result);
		    Tcl_DStringAppend(&reply, "\0-c ", 4);
		    Tcl_DStringAppend(&reply, buffer, -1);
		}
		(void) AppendPropCarefully(dispPtr->display, commWindow,
			dispPtr->commProperty, Tcl_DStringValue(&reply),
			Tcl_DStringLength(&reply) + 1, NULL);
		XFlush(dispPtr->display);
		Tcl_DStringFree(&reply);
	    }
	} else if ((*p == 'r') && (p[1] == 0)) {
	    int serial, code, gotSerial;
	    const char *errorInfo, *errorCode, *resultString;
	    PendingCommand *pcPtr;

	    /*
	     *----------------------------------------------------------
	     * This is a reply to some command that we sent out. Iterate over
	     * all of its options. Stop when we reach the end of the property
	     * or something that doesn't look like an option.
	     *----------------------------------------------------------
	     */

	    p += 2;
	    code = TCL_OK;
	    gotSerial = 0;
	    errorInfo = NULL;
	    errorCode = NULL;
	    resultString = "";
	    while (((p-propInfo) < (int) numItems) && (*p == '-')) {
		switch (p[1]) {
		case 'c':
		    if (sscanf(p+2, " %d", &code) != 1) {
			code = TCL_OK;
		    }
		    break;
		case 'e':
		    if (p[2] == ' ') {
			errorCode = p+3;
		    }
		    break;
		case 'i':
		    if (p[2] == ' ') {
			errorInfo = p+3;
		    }
		    break;
		case 'r':
		    if (p[2] == ' ') {
			resultString = p+3;
		    }
		    break;
		case 's':
		    if (sscanf(p+2, " %d", &serial) == 1) {
			gotSerial = 1;
		    }
		    break;
		}
		while (*p != 0) {
		    p++;
		}
		p++;
	    }

	    if (!gotSerial) {
		continue;
	    }

	    /*
	     * Give the result information to anyone who's waiting for it.
	     */

	    for (pcPtr = tsdPtr->pendingCommands; pcPtr != NULL;
		    pcPtr = pcPtr->nextPtr) {
		if ((serial != pcPtr->serial) || (pcPtr->result != NULL)) {
		    continue;
		}
		pcPtr->code = code;
		if (resultString != NULL) {
		    pcPtr->result = ckalloc(strlen(resultString) + 1);
		    strcpy(pcPtr->result, resultString);
		}
		if (code == TCL_ERROR) {
		    if (errorInfo != NULL) {
			pcPtr->errorInfo = ckalloc(strlen(errorInfo) + 1);
			strcpy(pcPtr->errorInfo, errorInfo);
		    }
		    if (errorCode != NULL) {
			pcPtr->errorCode = ckalloc(strlen(errorCode) + 1);
			strcpy(pcPtr->errorCode, errorCode);
		    }
		}
		pcPtr->gotResponse = 1;
		break;
	    }
	} else {
	    /*
	     * Didn't recognize this thing. Just skip through the next null
	     * character and try again.
	     */

	    while (*p != 0) {
		p++;
	    }
	    p++;
	}
    }
    XFree(propInfo);
}

/*
 *--------------------------------------------------------------
 *
 * AppendPropCarefully --
 *
 *	Append a given property to a given window, but set up an X error
 *	handler so that if the append fails this function can return an error
 *	code rather than having Xlib panic.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The given property on the given window is appended to. If this
 *	operation fails and if pendingPtr is non-NULL, then the pending
 *	operation is marked as complete with an error.
 *
 *--------------------------------------------------------------
 */

static void
AppendPropCarefully(
    Display *display,		/* Display on which to operate. */
    Window window,		/* Window whose property is to be modified. */
    Atom property,		/* Name of property. */
    char *value,		/* Characters to append to property. */
    int length,			/* Number of bytes to append. */
    PendingCommand *pendingPtr)	/* Pending command to mark complete if an
				 * error occurs during the property op. NULL
				 * means just ignore the error. */
{
    Tk_ErrorHandler handler;

    handler = Tk_CreateErrorHandler(display, -1, -1, -1, AppendErrorProc,
	    pendingPtr);
    XChangeProperty(display, window, property, XA_STRING, 8,
	    PropModeAppend, (unsigned char *) value, length);
    Tk_DeleteErrorHandler(handler);
}

/*
 * The function below is invoked if an error occurs during the XChangeProperty
 * operation above.
 */

	/* ARGSUSED */
static int
AppendErrorProc(
    ClientData clientData,	/* Command to mark complete, or NULL. */
    XErrorEvent *errorPtr)	/* Information about error. */
{
    PendingCommand *pendingPtr = clientData;
    register PendingCommand *pcPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (pendingPtr == NULL) {
	return 0;
    }

    /*
     * Make sure this command is still pending.
     */

    for (pcPtr = tsdPtr->pendingCommands; pcPtr != NULL;
	    pcPtr = pcPtr->nextPtr) {
	if ((pcPtr == pendingPtr) && (pcPtr->result == NULL)) {
	    pcPtr->result = ckalloc(strlen(pcPtr->target) + 50);
	    sprintf(pcPtr->result, "no application named \"%s\"",
		    pcPtr->target);
	    pcPtr->code = TCL_ERROR;
	    pcPtr->gotResponse = 1;
	    break;
	}
    }
    return 0;
}

/*
 *--------------------------------------------------------------
 *
 * DeleteProc --
 *
 *	This function is invoked by Tcl when the "send" command is deleted in
 *	an interpreter. It unregisters the interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter given by riPtr is unregistered.
 *
 *--------------------------------------------------------------
 */

static void
DeleteProc(
    ClientData clientData)	/* Info about registration, passed as
				 * ClientData. */
{
    RegisteredInterp *riPtr = clientData;
    register RegisteredInterp *riPtr2;
    NameRegistry *regPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    regPtr = RegOpen(riPtr->interp, riPtr->dispPtr, 1);
    RegDeleteName(regPtr, riPtr->name);
    RegClose(regPtr);

    if (tsdPtr->interpListPtr == riPtr) {
	tsdPtr->interpListPtr = riPtr->nextPtr;
    } else {
	for (riPtr2 = tsdPtr->interpListPtr; riPtr2 != NULL;
		riPtr2 = riPtr2->nextPtr) {
	    if (riPtr2->nextPtr == riPtr) {
		riPtr2->nextPtr = riPtr->nextPtr;
		break;
	    }
	}
    }
    ckfree(riPtr->name);
    riPtr->interp = NULL;
    UpdateCommWindow(riPtr->dispPtr);
    Tcl_EventuallyFree(riPtr, TCL_DYNAMIC);
}

/*
 *----------------------------------------------------------------------
 *
 * SendRestrictProc --
 *
 *	This function filters incoming events when a "send" command is
 *	outstanding. It defers all events except those containing send
 *	commands and results.
 *
 * Results:
 *	False is returned except for property-change events on a commWindow.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
static Tk_RestrictAction
SendRestrictProc(
    ClientData clientData,		/* Not used. */
    register XEvent *eventPtr)		/* Event that just arrived. */
{
    TkDisplay *dispPtr;

    if (eventPtr->type != PropertyNotify) {
	return TK_DEFER_EVENT;
    }
    for (dispPtr = TkGetDisplayList(); dispPtr != NULL;
	    dispPtr = dispPtr->nextPtr) {
	if ((eventPtr->xany.display == dispPtr->display)
		&& (eventPtr->xproperty.window
		== Tk_WindowId(dispPtr->commTkwin))) {
	    return TK_PROCESS_EVENT;
	}
    }
    return TK_DEFER_EVENT;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateCommWindow --
 *
 *	This function updates the list of application names stored on our
 *	commWindow. It is typically called when interpreters are registered
 *	and unregistered.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The TK_APPLICATION property on the comm window is updated.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateCommWindow(
    TkDisplay *dispPtr)		/* Display whose commWindow is to be
				 * updated. */
{
    Tcl_DString names;
    RegisteredInterp *riPtr;
    ThreadSpecificData *tsdPtr =
	    Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_DStringInit(&names);
    for (riPtr = tsdPtr->interpListPtr; riPtr != NULL;
	    riPtr = riPtr->nextPtr) {
	Tcl_DStringAppendElement(&names, riPtr->name);
    }
    XChangeProperty(dispPtr->display, Tk_WindowId(dispPtr->commTkwin),
	    dispPtr->appNameProperty, XA_STRING, 8, PropModeReplace,
	    (unsigned char *) Tcl_DStringValue(&names),
	    Tcl_DStringLength(&names));
    Tcl_DStringFree(&names);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpTestsendCmd --
 *
 *	This function implements the "testsend" command. It provides a set of
 *	functions for testing the "send" command and support function in
 *	tkSend.c.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on option; see below.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TkpTestsendCmd(
    ClientData clientData,	/* Main window for application. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument strings. */
{
    enum {
	TESTSEND_BOGUS, TESTSEND_PROP, TESTSEND_SERIAL
    };
    static const char *const testsendOptions[] = {
	"bogus",   "prop",   "serial",  NULL
    };
    TkWindow *winPtr = clientData;
    Tk_ErrorHandler handler;
    int index;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv,
		"option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObjStruct(interp, objv[1], testsendOptions,
		sizeof(char *), "option", 0, &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index == TESTSEND_BOGUS) {
        handler = Tk_CreateErrorHandler(winPtr->dispPtr->display, -1, -1, -1,
                NULL, NULL);
	XChangeProperty(winPtr->dispPtr->display,
		RootWindow(winPtr->dispPtr->display, 0),
		winPtr->dispPtr->registryProperty, XA_INTEGER, 32,
		PropModeReplace,
		(unsigned char *) "This is bogus information", 6);
        Tk_DeleteErrorHandler(handler);
    } else if (index == TESTSEND_PROP) {
	int result, actualFormat;
	unsigned long length, bytesAfter;
	Atom actualType, propName;
	char *property, **propertyPtr = &property, *p, *end;
	Window w;

	if ((objc != 4) && (objc != 5)) {
		Tcl_WrongNumArgs(interp, 1, objv,
			"prop window name ?value ?");
	    return TCL_ERROR;
	}
	if (strcmp(Tcl_GetString(objv[2]), "root") == 0) {
	    w = RootWindow(winPtr->dispPtr->display, 0);
	} else if (strcmp(Tcl_GetString(objv[2]), "comm") == 0) {
	    w = Tk_WindowId(winPtr->dispPtr->commTkwin);
	} else {
	    w = strtoul(Tcl_GetString(objv[2]), &end, 0);
	}
	propName = Tk_InternAtom((Tk_Window) winPtr, Tcl_GetString(objv[3]));
	if (objc == 4) {
	    property = NULL;
	    result = XGetWindowProperty(winPtr->dispPtr->display, w, propName,
		    0, 100000, False, XA_STRING, &actualType, &actualFormat,
		    &length, &bytesAfter, (unsigned char **) propertyPtr);
	    if ((result == Success) && (actualType != None)
		    && (actualFormat == 8) && (actualType == XA_STRING)) {
		for (p = property; (unsigned long)(p-property) < length; p++) {
		    if (*p == 0) {
			*p = '\n';
		    }
		}
		Tcl_SetObjResult(interp, Tcl_NewStringObj(property, -1));
	    }
	    if (property != NULL) {
		XFree(property);
	    }
	} else if (Tcl_GetString(objv[4])[0] == 0) {
            handler = Tk_CreateErrorHandler(winPtr->dispPtr->display,
                    -1, -1, -1, NULL, NULL);
	    XDeleteProperty(winPtr->dispPtr->display, w, propName);
            Tk_DeleteErrorHandler(handler);
	} else {
	    Tcl_DString tmp;

	    Tcl_DStringInit(&tmp);
	    for (p = Tcl_DStringAppend(&tmp, Tcl_GetString(objv[4]),
		    (int) strlen(Tcl_GetString(objv[4]))); *p != 0; p++) {
		if (*p == '\n') {
		    *p = 0;
		}
	    }
            handler = Tk_CreateErrorHandler(winPtr->dispPtr->display,
                    -1, -1, -1, NULL, NULL);
	    XChangeProperty(winPtr->dispPtr->display, w, propName, XA_STRING,
		    8, PropModeReplace, (unsigned char*)Tcl_DStringValue(&tmp),
		    p-Tcl_DStringValue(&tmp));
            Tk_DeleteErrorHandler(handler);
	    Tcl_DStringFree(&tmp);
	}
    } else if (index == TESTSEND_SERIAL) {
	Tcl_SetObjResult(interp, Tcl_NewIntObj(localData.sendSerial+1));
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
