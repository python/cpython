/*
 * tkMacOSXHLEvents.c --
 *
 *	Implements high level event support for the Macintosh. Currently, the
 *	only event that really does anything is the Quit event.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2015 Marc Culler
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include <sys/param.h>
#define URL_MAX_LENGTH (17 + MAXPATHLEN)

/*
 * This is a Tcl_Event structure that the Quit AppleEvent handler uses to
 * schedule the ReallyKillMe function.
 */

typedef struct KillEvent {
    Tcl_Event header;		/* Information that is standard for all
				 * events. */
    Tcl_Interp *interp;		/* Interp that was passed to the Quit
				 * AppleEvent */
} KillEvent;

/*
 * Static functions used only in this file.
 */

static void tkMacOSXProcessFiles(NSAppleEventDescriptor* event,
				 NSAppleEventDescriptor* replyEvent,
				 Tcl_Interp *interp,
				 const char* procedure);
static int  MissedAnyParameters(const AppleEvent *theEvent);
static int  ReallyKillMe(Tcl_Event *eventPtr, int flags);

#pragma mark TKApplication(TKHLEvents)

@implementation TKApplication(TKHLEvents)
- (void) terminate: (id) sender
{
    [self handleQuitApplicationEvent:Nil withReplyEvent:Nil];
}

- (void) preferences: (id) sender
{
    [self handleShowPreferencesEvent:Nil withReplyEvent:Nil];
}

- (void) handleQuitApplicationEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    KillEvent *eventPtr;

    if (_eventInterp) {
	/*
	 * Call the exit command from the event loop, since you are not
	 * supposed to call ExitToShell in an Apple Event Handler. We put this
	 * at the head of Tcl's event queue because this message usually comes
	 * when the Mac is shutting down, and we want to kill the shell as
	 * quickly as possible.
	 */

	eventPtr = ckalloc(sizeof(KillEvent));
	eventPtr->header.proc = ReallyKillMe;
	eventPtr->interp = _eventInterp;

	Tcl_QueueEvent((Tcl_Event *) eventPtr, TCL_QUEUE_HEAD);
    }
}

- (void) handleOpenApplicationEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
   Tcl_Interp *interp = _eventInterp;

    if (interp &&
	Tcl_FindCommand(_eventInterp, "::tk::mac::OpenApplication", NULL, 0)){
	int code = Tcl_EvalEx(_eventInterp, "::tk::mac::OpenApplication",
			      -1, TCL_EVAL_GLOBAL);
	if (code != TCL_OK) {
	    Tcl_BackgroundException(_eventInterp, code);
	}
    }
}

- (void) handleReopenApplicationEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
#if MAC_OS_X_VERSION_MIN_REQUIRED < 1090
    ProcessSerialNumber thePSN = {0, kCurrentProcess};
    SetFrontProcess(&thePSN);
#else
    [[NSApplication sharedApplication] activateIgnoringOtherApps: YES];
#endif
    if (_eventInterp && Tcl_FindCommand(_eventInterp,
	    "::tk::mac::ReopenApplication", NULL, 0)) {
	int code = Tcl_EvalEx(_eventInterp, "::tk::mac::ReopenApplication",
			      -1, TCL_EVAL_GLOBAL);
	if (code != TCL_OK){
	    Tcl_BackgroundException(_eventInterp, code);
	}
    }
}

- (void) handleShowPreferencesEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    if (_eventInterp &&
	    Tcl_FindCommand(_eventInterp, "::tk::mac::ShowPreferences", NULL, 0)){
	int code = Tcl_EvalEx(_eventInterp, "::tk::mac::ShowPreferences",
			      -1, TCL_EVAL_GLOBAL);
	if (code != TCL_OK) {
	    Tcl_BackgroundException(_eventInterp, code);
	}
    }
}

- (void) handleOpenDocumentsEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    tkMacOSXProcessFiles(event, replyEvent, _eventInterp, "::tk::mac::OpenDocument");
}

- (void) handlePrintDocumentsEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    tkMacOSXProcessFiles(event, replyEvent, _eventInterp, "::tk::mac::PrintDocument");
}

- (void) handleDoScriptEvent: (NSAppleEventDescriptor *)event
    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    OSStatus err;
    const AEDesc *theDesc = nil;
    DescType type = 0, initialType = 0;
    Size actual;
    int tclErr = -1;
    char URLBuffer[1 + URL_MAX_LENGTH];
    char errString[128];
    char typeString[5];

    /*
     * The DoScript event receives one parameter that should be text data or a
     * fileURL.
     */

    theDesc = [event aeDesc];
    if (theDesc == nil) {
	return;
    }

    err = AEGetParamPtr(theDesc, keyDirectObject, typeWildCard, &initialType,
			NULL, 0, NULL);
    if (err != noErr) {
	sprintf(errString, "AEDoScriptHandler: GetParamDesc error %d", (int)err);
	AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorString, typeChar,
		      errString, strlen(errString));
	return;
    }

    if (MissedAnyParameters((AppleEvent*)theDesc)) {
    	sprintf(errString, "AEDoScriptHandler: extra parameters");
    	AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorString, typeChar,
    		      errString, strlen(errString));
    	return;
    }

    if (initialType == typeFileURL || initialType == typeAlias) {
	/*
	 * The descriptor can be coerced to a file url.  Source the file, or
	 * pass the path as a string argument to ::tk::mac::DoScriptFile if
	 * that procedure exists.
	 */
	err = AEGetParamPtr(theDesc, keyDirectObject, typeFileURL, &type,
			    (Ptr) URLBuffer, URL_MAX_LENGTH, &actual);
	if (err == noErr && actual > 0){
	    URLBuffer[actual] = '\0';
	    NSString *urlString = [NSString stringWithUTF8String:(char*)URLBuffer];
	    NSURL *fileURL = [NSURL URLWithString:urlString];
	    Tcl_DString command;
	    Tcl_DStringInit(&command);
	    if (Tcl_FindCommand(_eventInterp, "::tk::mac::DoScriptFile", NULL, 0)){
		Tcl_DStringAppend(&command, "::tk::mac::DoScriptFile", -1);
	    } else {
		Tcl_DStringAppend(&command, "source", -1);
	    }
	    Tcl_DStringAppendElement(&command, [[fileURL path] UTF8String]);
	    tclErr = Tcl_EvalEx(_eventInterp, Tcl_DStringValue(&command),
				 Tcl_DStringLength(&command), TCL_EVAL_GLOBAL);
	}
    } else if (noErr == AEGetParamPtr(theDesc, keyDirectObject, typeUTF8Text, &type,
			   NULL, 0, &actual)) {
	if (actual > 0) {
	    /*
	     * The descriptor can be coerced to UTF8 text.  Evaluate as Tcl, or
	     * or pass the text as a string argument to ::tk::mac::DoScriptText
	     * if that procedure exists.
	     */
	    char *data = ckalloc(actual + 1);
	    if (noErr == AEGetParamPtr(theDesc, keyDirectObject, typeUTF8Text, &type,
			    data, actual, NULL)) {
		if (Tcl_FindCommand(_eventInterp, "::tk::mac::DoScriptText", NULL, 0)){
		    Tcl_DString command;
		    Tcl_DStringInit(&command);
		    Tcl_DStringAppend(&command, "::tk::mac::DoScriptText", -1);
		    Tcl_DStringAppendElement(&command, data);
		    tclErr = Tcl_EvalEx(_eventInterp, Tcl_DStringValue(&command),
				 Tcl_DStringLength(&command), TCL_EVAL_GLOBAL);
		} else {
		    tclErr = Tcl_EvalEx(_eventInterp, data, actual, TCL_EVAL_GLOBAL);
		}
	    }
	    ckfree(data);
	}
    } else {
	/*
	 * The descriptor can not be coerced to a fileURL or UTF8 text.
	 */
	for (int i = 0; i < 4; i++) {
	    typeString[i] = ((char*)&initialType)[3-i];
	}
	typeString[4] = '\0';
	sprintf(errString, "AEDoScriptHandler: invalid script type '%s', "
		"must be coercable to 'furl' or 'utf8'", typeString);
	AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorString, typeChar, errString,
		      strlen(errString));
    }
    /*
     * If we ran some Tcl code, put the result in the reply.
     */
    if (tclErr >= 0) {
	int reslen;
	const char *result =
	    Tcl_GetStringFromObj(Tcl_GetObjResult(_eventInterp), &reslen);
	if (tclErr == TCL_OK) {
	    AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyDirectObject, typeChar,
			  result, reslen);
	} else {
	    AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorString, typeChar,
			  result, reslen);
	    AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorNumber, typeSInt32,
			  (Ptr) &tclErr,sizeof(int));
	}
    }
    return;
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXProcessFiles --
 *
 *	Extract a list of fileURLs from an AppleEvent and call the specified
 *      procedure with the file paths as arguments.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The event is handled by running the procedure.
 *
 *----------------------------------------------------------------------
 */

static void
tkMacOSXProcessFiles(
    NSAppleEventDescriptor* event,
    NSAppleEventDescriptor* replyEvent,
    Tcl_Interp *interp,
    const char* procedure)
{
    Tcl_Encoding utf8;
    const AEDesc *fileSpecDesc = nil;
    AEDesc contents;
    char URLString[1 + URL_MAX_LENGTH];
    NSURL *fileURL;
    DescType type;
    Size actual;
    long count, index;
    AEKeyword keyword;
    Tcl_DString command, pathName;
    int code;

    /*
     * Do nothing if we don't have an interpreter or the procedure doesn't exist.
     */

    if (!interp || !Tcl_FindCommand(interp, procedure, NULL, 0)) {
	return;
    }

    fileSpecDesc = [event aeDesc];
    if (fileSpecDesc == nil ) {
    	return;
    }

    /*
     * The AppleEvent's descriptor should either contain a value of
     * typeObjectSpecifier or typeAEList.  In the first case, the descriptor
     * can be treated as a list of size 1 containing a value which can be
     * coerced into a fileURL. In the second case we want to work with the list
     * itself.  Values in the list will be coerced into fileURL's if possible;
     * otherwise they will be ignored.
     */

    /* Get a copy of the AppleEvent's descriptor. */
    AEGetParamDesc(fileSpecDesc, keyDirectObject, typeWildCard, &contents);
    if (contents.descriptorType == typeAEList) {
    	fileSpecDesc = &contents;
    }

    if (AECountItems(fileSpecDesc, &count) != noErr) {
	AEDisposeDesc(&contents);
    	return;
    }

    /*
     * Construct a Tcl command which calls the procedure, passing the
     * paths contained in the AppleEvent as arguments.
     */

    Tcl_DStringInit(&command);
    Tcl_DStringAppend(&command, procedure, -1);
    utf8 = Tcl_GetEncoding(NULL, "utf-8");

    for (index = 1; index <= count; index++) {
	if (noErr != AEGetNthPtr(fileSpecDesc, index, typeFileURL, &keyword,
				 &type, (Ptr) URLString, URL_MAX_LENGTH, &actual)) {
	    continue;
	}
	if (type != typeFileURL) {
	    continue;
	}
	URLString[actual] = '\0';
	fileURL = [NSURL URLWithString:[NSString stringWithUTF8String:(char*)URLString]];
	if (fileURL == nil) {
	    continue;
	}
	Tcl_ExternalToUtfDString(utf8, [[fileURL path] UTF8String], -1, &pathName);
	Tcl_DStringAppendElement(&command, Tcl_DStringValue(&pathName));
	Tcl_DStringFree(&pathName);
    }

    Tcl_FreeEncoding(utf8);
    AEDisposeDesc(&contents);

    /*
     * Handle the event by evaluating the Tcl expression we constructed.
     */

    code = Tcl_EvalEx(interp, Tcl_DStringValue(&command),
	    Tcl_DStringLength(&command), TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
	Tcl_BackgroundException(interp, code);
    }
    Tcl_DStringFree(&command);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXInitAppleEvents --
 *
 *	Register AppleEvent handlers with the NSAppleEventManager for
 *      this NSApplication.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXInitAppleEvents(
    Tcl_Interp *interp)   /* not used */
{
    NSAppleEventManager *aeManager = [NSAppleEventManager sharedAppleEventManager];
    static Boolean initialized = FALSE;

    if (!initialized) {
	initialized = TRUE;

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleQuitApplicationEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEQuitApplication];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleOpenApplicationEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEOpenApplication];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleReopenApplicationEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEReopenApplication];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleShowPreferencesEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEShowPreferences];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleOpenDocumentsEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEOpenDocuments];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleOpenDocumentsEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEPrintDocuments];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleDoScriptEvent:withReplyEvent:)
	    forEventClass:kAEMiscStandards andEventID:kAEDoScript];
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXDoHLEvent --
 *
 *	Dispatch an AppleEvent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depend on the AppleEvent.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXDoHLEvent(
    void *theEvent)
{
    /* According to the NSAppleEventManager reference:
     *   "The theReply parameter always specifies a reply Apple event, never
     *   nil.  However, the handler should not fill out the reply if the
     *   descriptor type for the reply event is typeNull, indicating the sender
     *   does not want a reply."
     * The specified way to build such a non-nil descriptor is used here.  But
     * on OSX 10.11, the compiler nonetheless generates a warning.  I am
     * supressing the warning here -- maybe the warnings will stop in a future
     * compiler release.
     */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnonnull"
#endif

    NSAppleEventDescriptor* theReply = [NSAppleEventDescriptor nullDescriptor];
    NSAppleEventManager *aeManager = [NSAppleEventManager sharedAppleEventManager];

    return [aeManager dispatchRawAppleEvent:(const AppleEvent*)theEvent
		      withRawReply: (AppleEvent *)theReply
		      handlerRefCon: (SRefCon)0];

#ifdef __clang__
#pragma clang diagnostic pop
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * ReallyKillMe --
 *
 *	This procedure tries to kill the shell by running exit, called from
 *      an event scheduled by the "Quit" AppleEvent handler.
 *
 * Results:
 *	Runs the "exit" command which might kill the shell.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
ReallyKillMe(
    Tcl_Event *eventPtr,
    int flags)
{
    Tcl_Interp *interp = ((KillEvent *) eventPtr)->interp;
    int quit = Tcl_FindCommand(interp, "::tk::mac::Quit", NULL, 0)!=NULL;
    int code = Tcl_EvalEx(interp, quit ? "::tk::mac::Quit" : "exit", -1, TCL_EVAL_GLOBAL);

    if (code != TCL_OK) {
	/*
	 * Should be never reached...
	 */

	Tcl_BackgroundException(interp, code);
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * MissedAnyParameters --
 *
 *	Checks to see if parameters are still left in the event.
 *
 * Results:
 *	True or false.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
MissedAnyParameters(
    const AppleEvent *theEvent)
{
   DescType returnedType;
   Size actualSize;
   OSStatus err;

   err = AEGetAttributePtr(theEvent, keyMissedKeywordAttr,
	    typeWildCard, &returnedType, NULL, 0, &actualSize);

   return (err != errAEDescNotFound);
}


/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
