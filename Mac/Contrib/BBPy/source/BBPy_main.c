/*	BBPython
	A simple menu command to send the contents of a window to the Python interpreter
	
	copyright © 1996 Just van Rossum, Letterror: just@knoware.nl
	
	All Rights Reserved
*/

#include "BBPy.h"

OSErr SendTextAsAE(ExternalCallbackBlock *callbacks, Ptr theText, long theSize, Str255 windowTitle)
{
	OSErr			err;
	AEDesc		theEvent;
	AEAddressDesc	theTarget;
	AppleEvent	theReply;
	AEDesc		theTextDesc;
	AEDesc		theNameDesc;
	OSType		pythonSig = 'Pyth';
	FSSpec		docSpec;
	short			itemHit;
	long			time;
	EventRecord	theDummyEvent;
	
	/* initialize AE descriptor for python's signature */
	err = AECreateDesc (typeApplSignature, &pythonSig, sizeof(OSType), &theTarget);
	if(err != noErr) return err;
	
	/* initialize AE descriptor for the title of our window */
	err = AECreateDesc (typeChar, &windowTitle[1], windowTitle[0], &theNameDesc);
	if(err != noErr) return err;
	
	/* initialize AE descriptor for the content of our window */
	err = AECreateDesc ('TEXT', theText, theSize, &theTextDesc);
	if(err != noErr) return err;
	
	/* initialize AppleEvent */
	err = AECreateAppleEvent ('pyth', 'EXEC', &theTarget, kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
	if(err != noErr) return err;
	
	/* add the content of our window to the AppleEvent */
	err = AEPutParamDesc (&theEvent, keyDirectObject, &theTextDesc);
	if(err != noErr) return err;
	
	/* add the title of our window to the AppleEvent */
	err = AEPutParamDesc (&theEvent, 'NAME', &theNameDesc);
	if(err != noErr) return err;
	
	/* send the AppleEvent */
	err = AESend (&theEvent, &theReply, kAEWaitReply, kAEHighPriority, kNoTimeOut, NULL, NULL);
	if(err == connectionInvalid) {
		// launch PythonSlave.py
		itemHit = Alert(128, NULL);
		if(itemHit == 2)  return noErr;	/* user cancelled */
		
		if( ! GetPythonSlaveSpec(&docSpec) )
			return noErr;		/* user cancelled */
		
		err = LaunchPythonSlave(&docSpec);
		if(err != noErr) return err;
	} else if(err != noErr) 
		return err;
	
	/* clean up */
	err = AEDisposeDesc (&theTarget);
	if(err != noErr) return err;
	
	err = AEDisposeDesc (&theNameDesc);
	if(err != noErr) return err;
	
	err = AEDisposeDesc (&theTextDesc);
	if(err != noErr) return err;
	
	err = AEDisposeDesc (&theEvent);
	if(err != noErr) return err;
	
	err = AEDisposeDesc (&theReply);
	if(err != noErr) return err;
	
	/* everything is cool */
	return noErr;
}

pascal void main(ExternalCallbackBlock *callbacks, WindowPtr theWindow)
{
	long 		oldA4;
	OSErr		err;
	Handle	windowContents;
	Str255	windowTitle;
	
	//RememberA0(); /* Can't find header file for this. Seems to work anyway. */
	
	oldA4 = SetUpA4();	

	GetWTitle(theWindow, windowTitle);
	windowContents = callbacks->GetWindowContents(theWindow);
	
	HLock(windowContents);
	err = SendTextAsAE(callbacks, *windowContents, GetHandleSize(windowContents), windowTitle);
	if(err != noErr) callbacks->ReportOSError(err);
	HUnlock(windowContents);
	
	RestoreA4(oldA4);
}
