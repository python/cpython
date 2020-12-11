/*
 * tkMacOSXHLEvents.c --
 *
 *	Implements high level event support for the Macintosh.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2015-2019 Marc Culler
 * Copyright (c) 2019 Kevin Walzer/WordTech Communications LLC.
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
 * When processing an AppleEvent as an idle task, a pointer to one
 * of these structs is passed as the clientData.
 */

typedef struct AppleEventInfo {
    Tcl_Interp *interp;
    const char *procedure;
    Tcl_DString command;
    NSAppleEventDescriptor *replyEvent; /* Only used for DoScriptText. */
} AppleEventInfo;

/*
 * Static functions used only in this file.
 */

static int  MissedAnyParameters(const AppleEvent *theEvent);
static int  ReallyKillMe(Tcl_Event *eventPtr, int flags);
static void ProcessAppleEvent(ClientData clientData);

/*
 * Names of the procedures which can be used to process AppleEvents.
 */

static const char *openDocumentProc = "::tk::mac::OpenDocument";
static const char *launchURLProc = "::tk::mac::LaunchURL";
static const char *printDocProc = "::tk::mac::PrintDocument";
static const char *scriptFileProc = "::tk::mac::DoScriptFile";
static const char *scriptTextProc = "::tk::mac::DoScriptText";

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
    if (_eventInterp &&
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
    [NSApp activateIgnoringOtherApps: YES];
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
    Tcl_Encoding utf8;
    const AEDesc *fileSpecDesc = nil;
    AEDesc contents;
    char URLString[1 + URL_MAX_LENGTH];
    NSURL *fileURL;
    DescType type;
    Size actual;
    long count, index;
    AEKeyword keyword;
    Tcl_DString pathName;

    /*
     * Do nothing if we don't have an interpreter.
     */

    if (!_eventInterp) {
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
     * Construct a Tcl expression which calls the ::tk::mac::OpenDocument
     * procedure, passing the paths contained in the AppleEvent as arguments.
     */

    AppleEventInfo *AEInfo = ckalloc(sizeof(AppleEventInfo));
    Tcl_DString *openCommand = &AEInfo->command;
    Tcl_DStringInit(openCommand);
    Tcl_DStringAppend(openCommand, openDocumentProc, -1);
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
	Tcl_DStringAppendElement(openCommand, Tcl_DStringValue(&pathName));
	Tcl_DStringFree(&pathName);
    }

    Tcl_FreeEncoding(utf8);
    AEDisposeDesc(&contents);
    AEInfo->interp = _eventInterp;
    AEInfo->procedure = openDocumentProc;
    AEInfo->replyEvent = nil;
    Tcl_DoWhenIdle(ProcessAppleEvent, (ClientData)AEInfo);
}

- (void) handlePrintDocumentsEvent: (NSAppleEventDescriptor *)event
		    withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    NSString* file = [[event paramDescriptorForKeyword:keyDirectObject]
			 stringValue];
    const char *printFile = [file UTF8String];
    AppleEventInfo *AEInfo = ckalloc(sizeof(AppleEventInfo));
    Tcl_DString *printCommand = &AEInfo->command;
    Tcl_DStringInit(printCommand);
    Tcl_DStringAppend(printCommand, printDocProc, -1);
    Tcl_DStringAppendElement(printCommand, printFile);
    AEInfo->interp = _eventInterp;
    AEInfo->procedure = printDocProc;
    AEInfo->replyEvent = nil;
    Tcl_DoWhenIdle(ProcessAppleEvent, (ClientData)AEInfo);
}

- (void) handleDoScriptEvent: (NSAppleEventDescriptor *)event
	      withReplyEvent: (NSAppleEventDescriptor *)replyEvent
{
    OSStatus err;
    const AEDesc *theDesc = nil;
    DescType type = 0, initialType = 0;
    Size actual;
    char URLBuffer[1 + URL_MAX_LENGTH];
    char errString[128];

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
	AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorString,
		      typeChar, errString, strlen(errString));
	return;
    }

    if (MissedAnyParameters((AppleEvent*)theDesc)) {
    	sprintf(errString, "AEDoScriptHandler: extra parameters");
    	AEPutParamPtr((AppleEvent*)[replyEvent aeDesc], keyErrorString,
		      typeChar,errString, strlen(errString));
    	return;
    }

    if (initialType == typeFileURL || initialType == typeAlias) {

	/*
	 * This descriptor can be coerced to a file url.  Construct a Tcl
	 * expression which passes the file path as a string argument to
         * ::tk::mac::DoScriptFile.
	 */

	if (noErr == AEGetParamPtr(theDesc, keyDirectObject, typeFileURL, &type,
                                  (Ptr) URLBuffer, URL_MAX_LENGTH, &actual)) {
            if (actual > 0) {
                URLBuffer[actual] = '\0';
                NSString *urlString = [NSString stringWithUTF8String:(char*)URLBuffer];
                NSURL *fileURL = [NSURL URLWithString:urlString];
                AppleEventInfo *AEInfo = ckalloc(sizeof(AppleEventInfo));
                Tcl_DString *scriptFileCommand = &AEInfo->command;
                Tcl_DStringInit(scriptFileCommand);
                Tcl_DStringAppend(scriptFileCommand, scriptFileProc, -1);
                Tcl_DStringAppendElement(scriptFileCommand, [[fileURL path] UTF8String]);
                AEInfo->interp = _eventInterp;
                AEInfo->procedure = scriptFileProc;
                AEInfo->replyEvent = nil;
                Tcl_DoWhenIdle(ProcessAppleEvent, (ClientData)AEInfo);
            }
        }
    } else if (noErr == AEGetParamPtr(theDesc, keyDirectObject, typeUTF8Text, &type,
			       NULL, 0, &actual)) {
        /*
         * The descriptor cannot be coerced to a file URL but can be coerced to
         * text.  Construct a Tcl expression which passes the text as a string
         * argument to ::tk::mac::DoScriptText.
         */

	if (actual > 0) {
	    char *data = ckalloc(actual + 1);
	    if (noErr == AEGetParamPtr(theDesc, keyDirectObject,
				       typeUTF8Text, &type,
				       data, actual, NULL)) {
                AppleEventInfo *AEInfo = ckalloc(sizeof(AppleEventInfo));
                Tcl_DString *scriptTextCommand = &AEInfo->command;
                Tcl_DStringInit(scriptTextCommand);
                Tcl_DStringAppend(scriptTextCommand, scriptTextProc, -1);
		Tcl_DStringAppendElement(scriptTextCommand, data);
		AEInfo->interp = _eventInterp;
		AEInfo->procedure = scriptTextProc;
                if (Tcl_FindCommand(AEInfo->interp, AEInfo->procedure, NULL, 0)) {
                    AEInfo->replyEvent = replyEvent;
                    ProcessAppleEvent((ClientData)AEInfo);
                } else {
                    AEInfo->replyEvent = nil;
                    Tcl_DoWhenIdle(ProcessAppleEvent, (ClientData)AEInfo);
                }
	    }
	}
    }
}

- (void)handleURLEvent:(NSAppleEventDescriptor*)event
        withReplyEvent:(NSAppleEventDescriptor*)replyEvent
{
    NSString* url = [[event paramDescriptorForKeyword:keyDirectObject]
                        stringValue];
    const char *cURL=[url UTF8String];
    AppleEventInfo *AEInfo = ckalloc(sizeof(AppleEventInfo));
    Tcl_DString *launchCommand = &AEInfo->command;
    Tcl_DStringInit(launchCommand);
    Tcl_DStringAppend(launchCommand, launchURLProc, -1);
    Tcl_DStringAppendElement(launchCommand, cURL);
    AEInfo->interp = _eventInterp;
    AEInfo->procedure = launchURLProc;
    AEInfo->replyEvent = nil;
    Tcl_DoWhenIdle(ProcessAppleEvent, (ClientData)AEInfo);
}

@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * ProcessAppleEvent --
 *
 *      Usually used as an idle task which evaluates a Tcl expression generated
 *      from an AppleEvent.  If the AppleEventInfo passed as the client data
 *      has a non-null replyEvent, the result of evaluating the expression will
 *      be added to the reply.  This must not be done when this function is
 *      called as an idle task, but is done when handling DoScriptText events
 *      when this function is called directly.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The expression will be evaluated and the clientData will be freed.
 *      The replyEvent may be modified to contain the result of evaluating
 *      a Tcl expression.
 *
 *----------------------------------------------------------------------
 */

static void ProcessAppleEvent(
    ClientData clientData)
{
    int code;
    AppleEventInfo *AEInfo = (AppleEventInfo*) clientData;
    if (!AEInfo->interp ||
        !Tcl_FindCommand(AEInfo->interp, AEInfo->procedure, NULL, 0)) {
	return;
    }
    code = Tcl_EvalEx(AEInfo->interp, Tcl_DStringValue(&AEInfo->command),
	    Tcl_DStringLength(&AEInfo->command), TCL_EVAL_GLOBAL);
    if (code != TCL_OK) {
	Tcl_BackgroundException(AEInfo->interp, code);
    }

    if (AEInfo->replyEvent && code >= 0) {
        int reslen;
        const char *result = Tcl_GetStringFromObj(Tcl_GetObjResult(AEInfo->interp),
                                                  &reslen);
        if (code == TCL_OK) {
            AEPutParamPtr((AppleEvent*)[AEInfo->replyEvent aeDesc],
                          keyDirectObject, typeChar, result, reslen);
        } else {
            AEPutParamPtr((AppleEvent*)[AEInfo->replyEvent aeDesc],
                          keyErrorString, typeChar, result, reslen);
            AEPutParamPtr((AppleEvent*)[AEInfo->replyEvent aeDesc],
                          keyErrorNumber, typeSInt32, (Ptr) &code, sizeof(int));
        }
    }
    Tcl_DStringFree(&AEInfo->command);
    ckfree(clientData);
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
	    andSelector:@selector(handlePrintDocumentsEvent:withReplyEvent:)
	    forEventClass:kCoreEventClass andEventID:kAEPrintDocuments];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleDoScriptEvent:withReplyEvent:)
	    forEventClass:kAEMiscStandards andEventID:kAEDoScript];

	[aeManager setEventHandler:NSApp
	    andSelector:@selector(handleURLEvent:withReplyEvent:)
	    forEventClass:kInternetEventClass andEventID:kAEGetURL];

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
