/*
 *  doscript.c
 *  PythonLauncher
 *
 *  Created by Jack Jansen on Wed Jul 31 2002.
 *  Copyright (c) 2002 __MyCompanyName__. All rights reserved.
 *
 */

#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>
#import "doscript.h"

extern int
doscript(const char *command)
{
    char *bundleID = "com.apple.Terminal";
    AppleEvent evt, res;
    AEDesc desc;
    OSStatus err;

    [[NSWorkspace sharedWorkspace] launchApplication:@"/Applications/Utilities/Terminal.app/"];

    // Build event
    err = AEBuildAppleEvent(kAECoreSuite, kAEDoScript,
                             typeApplicationBundleID,
                             bundleID, strlen(bundleID),
                             kAutoGenerateReturnID,
                             kAnyTransactionID,
                             &evt, NULL,
                             "'----':utf8(@)", strlen(command),
                             command);
    if (err) {
        NSLog(@"AEBuildAppleEvent failed: %ld\n", (long)err);
        return err;
    }

    // Send event and check for any Apple Event Manager errors
    err = AESendMessage(&evt, &res, kAEWaitReply, kAEDefaultTimeout);
    AEDisposeDesc(&evt);
    if (err) {
        NSLog(@"AESendMessage failed: %ld\n", (long)err);
        return err;
    }
    // Check for any application errors
    err = AEGetParamDesc(&res, keyErrorNumber, typeSInt32, &desc);
    AEDisposeDesc(&res);
    if (!err) {
        AEGetDescData(&desc, &err, sizeof(err));
        NSLog(@"Terminal returned an error: %ld", (long)err);
        AEDisposeDesc(&desc);
    } else if (err == errAEDescNotFound) {
        err = noErr;
    } else {
        NSLog(@"AEGetPArmDesc returned an error: %ld", (long)err);
    }

    return err;
}
