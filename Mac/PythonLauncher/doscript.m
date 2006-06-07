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

/* I assume I could pick these up from somewhere, but where... */
#define CREATOR 'trmx'

#define ACTIVATE_CMD 'misc'
#define ACTIVATE_SUITE 'actv'

#define DOSCRIPT_CMD 'dosc'
#define DOSCRIPT_SUITE 'core'
#define WITHCOMMAND 'cmnd'

/* ... and there's probably also a better way to do this... */
#define START_TERMINAL "/Applications/Utilities/Terminal.app/Contents/MacOS/Terminal &"

extern int 
doscript(const char *command)
{
    OSErr err;
    AppleEvent theAEvent, theReply;
    AEAddressDesc terminalAddress;
    AEDesc commandDesc;
    OSType terminalCreator = CREATOR;
    
    /* set up locals  */
    AECreateDesc(typeNull, NULL, 0, &theAEvent);
    AECreateDesc(typeNull, NULL, 0, &terminalAddress);
    AECreateDesc(typeNull, NULL, 0, &theReply);
    AECreateDesc(typeNull, NULL, 0, &commandDesc);
    
    /* create the "activate" event for Terminal */
    err = AECreateDesc(typeApplSignature, (Ptr) &terminalCreator,
            sizeof(terminalCreator), &terminalAddress);
    if (err != noErr) {
        NSLog(@"doscript: AECreateDesc: error %d\n", err);
        goto bail;
    }
    err = AECreateAppleEvent(ACTIVATE_SUITE, ACTIVATE_CMD,
            &terminalAddress, kAutoGenerateReturnID,
            kAnyTransactionID, &theAEvent);
    
    if (err != noErr) {
        NSLog(@"doscript: AECreateAppleEvent(activate): error %d\n", err);
        goto bail;
    }
    /* send the event  */
    err = AESend(&theAEvent, &theReply, kAEWaitReply,
            kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
    if ( err == -600 ) {
        int count=10;
        /* If it failed with "no such process" try to start Terminal */
        err = system(START_TERMINAL);
        if ( err ) {
            NSLog(@"doscript: system(): %s\n", strerror(errno));
            goto bail;
        }
        do {
            sleep(1);
            /* send the event again */
            err = AESend(&theAEvent, &theReply, kAEWaitReply,
                    kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
        } while (err == -600 && --count > 0);
        if ( err == -600 )
            NSLog(@"doscript: Could not activate Terminal\n");
    }
    if (err != noErr) {
        NSLog(@"doscript: AESend(activate): error %d\n", err);
        goto bail;
    }
            
    /* create the "doscript with command" event for Terminal */
    err = AECreateAppleEvent(DOSCRIPT_SUITE, DOSCRIPT_CMD,
            &terminalAddress, kAutoGenerateReturnID,
            kAnyTransactionID, &theAEvent);
    if (err != noErr) {
        NSLog(@"doscript: AECreateAppleEvent(doscript): error %d\n", err);
        goto bail;
    }
    
    /* add the command to the apple event */
    err = AECreateDesc(typeChar, command, strlen(command), &commandDesc);
    if (err != noErr) {
        NSLog(@"doscript: AECreateDesc(command): error %d\n", err);
        goto bail;
    }
    err = AEPutParamDesc(&theAEvent, WITHCOMMAND, &commandDesc);
    if (err != noErr) {
        NSLog(@"doscript: AEPutParamDesc: error %d\n", err);
        goto bail;
    }

    /* send the command event to Terminal.app */
    err = AESend(&theAEvent, &theReply, kAEWaitReply,
            kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
    
    if (err != noErr) {
        NSLog(@"doscript: AESend(docommand): error %d\n", err);
        goto bail;
    }
    /* clean up and leave */
bail:
    AEDisposeDesc(&commandDesc);
    AEDisposeDesc(&theAEvent);
    AEDisposeDesc(&terminalAddress);
    AEDisposeDesc(&theReply);
    return err;
}
