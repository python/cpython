/*
 * tkMacOSXServices.c --
 *\
 *	This file allows the integration of Tk and the Cocoa NSServices API.
 *
 * Copyright (c) 2010-2019 Kevin Walzer/WordTech Communications LLC.
 * Copyright (c) 2019 Marc Culler.
 * Copyright (c) 2010 Adrian Robert.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <tkInt.h>
#include <tkMacOSXInt.h>

/*
 * Event proc which calls the PerformService procedure.
 */

static int
ServicesEventProc(
    Tcl_Event *event,
    int flags)
{
    TkMainInfo *info = TkGetMainInfoList();
    Tcl_GlobalEval(info->interp, "::tk::mac::PerformService");
    return 1;
}

/*
 * The Wish application can send the current selection in the Tk clipboard
 * to other applications which accept messages of type NSString.  The TkService
 * object provides this service via its provideService method.  (The method
 * must be specified in the application's Info.plist file for this to work.)
 */

@interface TkService : NSObject {

}

+ (void) initialize;
- (void) provideService:(NSPasteboard *)pboard
	       userData:(NSString *)data
		  error:(NSString **)error;
- (BOOL)writeSelectionToPasteboard:(NSPasteboard *)pboard
			     types:(NSArray *)types;

@end

/*
 * Class methods.
 */

@implementation TkService

+ (void) initialize {
    NSArray *sendTypes = [NSArray arrayWithObjects:@"NSStringPboardType",
				  @"NSPasteboardTypeString", nil];
    [NSApp registerServicesMenuSendTypes:sendTypes returnTypes:sendTypes];
    return;
}

/*
 * Get the current Tk selection and copy it to the system pasteboard.
 */

- (BOOL)writeSelectionToPasteboard:(NSPasteboard *)pboard
			     types:(NSArray *)types
{
    NSArray *typesDeclared = nil;
    NSString *pboardType = nil;
    TkMainInfo *info = TkGetMainInfoList();

    for (NSString *typeString in types) {
	if ([typeString isEqualToString:@"NSStringPboardType"] ||
	    [typeString isEqualToString:@"NSPasteboardTypeString"]) {
	    typesDeclared = [NSArray arrayWithObject:typeString];
	    pboardType = typeString;
	    break;
	}
    }
    if (!typesDeclared) {
	return NO;
    }
    Tcl_Eval(info->interp, "selection get");

    char *copystring = Tcl_GetString(Tcl_GetObjResult(info->interp));
    NSString *writestring = [NSString stringWithUTF8String:copystring];

    [pboard declareTypes:typesDeclared owner:nil];
    return [pboard setString:writestring forType:pboardType];
}

/*
 * This is the method that actually calls the Tk service; it must be specified
 * in Info.plist.
 */

- (void)provideService:(NSPasteboard *)pboard
	      userData:(NSString *)data
		 error:(NSString **)error
{
    NSString *pboardString = nil, *pboardType = nil;
    NSArray *types = [pboard types];
    Tcl_Event *event;

    /*
     * Get a string from the private pasteboard and copy it to the general
     * pasteboard to make it available to other applications.
     */

    for (NSString *typeString in types) {
	if ([typeString isEqualToString:@"NSStringPboardType"] ||
	    [typeString isEqualToString:@"NSPasteboardTypeString"]) {
	    pboardString = [pboard stringForType:typeString];
	    pboardType = typeString;
	    break;
	}
    }
    if (pboardString) {
	NSPasteboard *generalpasteboard = [NSPasteboard generalPasteboard];
	[generalpasteboard declareTypes:[NSArray arrayWithObjects:pboardType, nil]
				  owner:nil];
	[generalpasteboard setString:pboardString forType:pboardType];
	event = ckalloc(sizeof(Tcl_Event));
	event->proc = ServicesEventProc;
	Tcl_QueueEvent((Tcl_Event *)event, TCL_QUEUE_TAIL);
    }
}
@end

/*
 * Instantiate a TkService object and register it with the NSApplication.
 * This is called exactly one time from TkpInit.
 */

int
TkMacOSXServices_Init(
    Tcl_Interp *interp)
{
    /*
     * Initialize an instance of TkService and register it with the NSApp.
     */

    TkService *service = [[TkService alloc] init];
    [NSApp setServicesProvider:service];
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
