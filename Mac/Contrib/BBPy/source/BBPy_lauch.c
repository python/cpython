/*
 * 	Launch the PythonSlave.py script.
 * 	This works exactly as if you'd double clicked on the file in the Finder, which
 * 	not surprisingly is how its implemented (via the AppleEvents route of course).
 *	
 *	Largely based on code submitted by Mark Roseman <roseman@cpsc.ucalgary.ca>
 *	Thanks!
 */

#include "BBPy.h"

pascal Boolean MyFileFilter(CInfoPBPtr PB);
FileFilterUPP gMyFileFilterUPP = NULL;

Boolean GetPythonSlaveSpec(FSSpec * docSpec) {
 	StandardFileReply	reply;
	SFTypeList		typeList;
	
	typeList[0] = 'TEXT';
	
	//if (!gMyFileFilterUPP)
		gMyFileFilterUPP = NewFileFilterProc( MyFileFilter );
	
	StandardGetFile(gMyFileFilterUPP, 0, typeList, &reply);
	
	DisposePtr((Ptr)gMyFileFilterUPP);
	
	if(!reply.sfGood)
		return 0; /* user cancelled */
	
	docSpec->vRefNum = reply.sfFile.vRefNum;
	docSpec->parID = reply.sfFile.parID;
	BlockMove(reply.sfFile.name, docSpec->name, 64);
	return 1;
}

pascal Boolean MyFileFilter(CInfoPBPtr PB) {
	OSType	fType;	/* file type */
	OSType	fCreator;	/* file creator */
	
	fType =((HParmBlkPtr)PB)->fileParam.ioFlFndrInfo.fdType;
	fCreator = ((HParmBlkPtr)PB)->fileParam.ioFlFndrInfo.fdCreator;
	
	if (fType == 'TEXT' && 
			fCreator == 'Pyth')
		return 0;
	return 1;
}

OSErr LaunchPythonSlave(FSSpec * docSpec) {
	OSErr 			err;
	FSSpec 			dirSpec;
	AEAddressDesc 		finderAddress;
	AppleEvent 		theEvent, theReply;
	OSType			finderSig = 'MACS';
	AliasHandle 		DirAlias, FileAlias;
	AEDesc 			fileList;
	AEDesc 			aeDirDesc, listElem;
 	
	err = AECreateDesc(typeApplSignature, (Ptr)&finderSig, 4, &finderAddress);
	if(err != noErr) return err;
	
	err = AECreateAppleEvent('FNDR', 'sope', &finderAddress,
			kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
	if(err != noErr) return err;
	
	FSMakeFSSpec(docSpec->vRefNum, docSpec->parID, NULL, &dirSpec);
	NewAlias(NULL, &dirSpec, &DirAlias);
	NewAlias(NULL, docSpec, &FileAlias);
	err = AECreateList(NULL, 0, 0, &fileList);
	HLock((Handle)DirAlias);
	AECreateDesc(typeAlias, (Ptr)*DirAlias, GetHandleSize((Handle)DirAlias), &aeDirDesc);
	HUnlock((Handle)DirAlias);
	if ((err = AEPutParamDesc(&theEvent, keyDirectObject, &aeDirDesc)) == noErr) {
		AEDisposeDesc(&aeDirDesc);
		HLock((Handle)FileAlias);
		AECreateDesc(typeAlias, (Ptr)*FileAlias, GetHandleSize((Handle)FileAlias), &listElem);
		HLock((Handle)FileAlias);
		err = AEPutDesc(&fileList, 0, &listElem);
	}
	AEDisposeDesc(&listElem);
	err = AEPutParamDesc(&theEvent, 'fsel', &fileList);
	AEDisposeDesc(&fileList);
		
	err = AESend(&theEvent, &theReply, kAENoReply+kAENeverInteract,
			kAENormalPriority, kAEDefaultTimeout, 0L, 0L);
	if(err != noErr) return err;
	
	err = AEDisposeDesc(&theEvent);
	if(err != noErr) return err;
	
	err = AEDisposeDesc(&theReply);
	return err;
}
