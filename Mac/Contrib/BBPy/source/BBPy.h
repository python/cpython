/*	BBPython
	A simple menu command to send the contents of a window to the Python interpreter
	
	copyright © 1996 Just van Rossum, Letterror: just@knoware.nl
	
	All Rights Reserved
*/

#include <MacHeaders68K>
#include <A4Stuff.h>
#include <SetUpA4.h> // for global variables, multiple segments, etc.
#include "ExternalInterface.h"
#include <Memory.h>

extern OSErr SendTextAsAE(ExternalCallbackBlock *callbacks, Ptr theText, long theSize, Str255 windowTitle);
extern OSErr LaunchPythonSlave(FSSpec * docSpec);
extern Boolean GetPythonSlaveSpec(FSSpec * docSpec);
