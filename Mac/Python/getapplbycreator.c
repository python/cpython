/*
** FindApplicationFromCreator uses the Desktop Database to
** locate the creator application for the given document
**
** this routine will check the desktop database of all local
** disks, then the desktop databases of all server volumes
** (so up to two passes will be made)
**
** This code was created from FindApplicationFromDocument
** routine, origin unknown.
*/

#include <Types.h>
#include <Files.h>
#include <Errors.h>
#include "getapplbycreator.h"


OSErr FindApplicationFromCreator(OSType creator,
        FSSpecPtr applicationFSSpecPtr)
{
        enum { localPass, remotePass, donePass } volumePass;
        DTPBRec desktopParams;
        HParamBlockRec hfsParams;
        short volumeIndex;
        Boolean foundFlag;
        GetVolParmsInfoBuffer volumeInfoBuffer;
        OSErr retCode;

/* dkj 12/94 initialize flag to false (thanks to Peter Baral for pointing out this bug) */
        foundFlag = false;

        volumePass = localPass;
        volumeIndex = 0;

        do {
				/*
                ** first, find the vRefNum of the volume whose Desktop Database
                ** we're checking this time
				*/

				volumeIndex++;

				/*	convert the volumeIndex into a vRefNum */

				hfsParams.volumeParam.ioNamePtr = nil;
				hfsParams.volumeParam.ioVRefNum = 0;
				hfsParams.volumeParam.ioVolIndex = volumeIndex;
				retCode = PBHGetVInfoSync(&hfsParams);

				/* a nsvErr indicates that the current pass is over */
				if (retCode == nsvErr) goto SkipThisVolume;
				if (retCode != noErr) goto Bail;

				/*
				** call GetVolParms to determine if this volume is a server
				** (a remote volume)
				*/
				
				hfsParams.ioParam.ioBuffer = (Ptr) &volumeInfoBuffer;
				hfsParams.ioParam.ioReqCount = sizeof(GetVolParmsInfoBuffer);
				retCode = PBHGetVolParmsSync(&hfsParams);
				if (retCode != noErr) goto Bail;
				
				/*
				** if the vMServerAdr field of the volume information buffer
				** is zero, this is a local volume; skip this volume
				** if it's local on a remote pass or remote on a local pass
				*/
				
				if ((volumeInfoBuffer.vMServerAdr != 0) !=
						(volumePass == remotePass)) goto SkipThisVolume;

				/* okay, now we've found the vRefNum for our desktop database call */

				desktopParams.ioVRefNum = hfsParams.volumeParam.ioVRefNum;

				/*
                ** find the path refNum for the desktop database for
                ** the volume we're interested in
                */

                desktopParams.ioNamePtr = nil;

                retCode = PBDTGetPath(&desktopParams);
                if (retCode == noErr && desktopParams.ioDTRefNum != 0) {

						/*
                        ** use the GetAPPL call to find the preferred application
                        ** for opening any document with this one's creator
                        */

                        desktopParams.ioIndex = 0;
                        desktopParams.ioFileCreator = creator;
                        desktopParams.ioNamePtr = applicationFSSpecPtr->name;
                        retCode = PBDTGetAPPLSync(&desktopParams);

                        if (retCode == noErr) {
								/*
                                ** okay, found it; fill in the application file spec
                                ** and set the flag indicating we're done
                                */

                                applicationFSSpecPtr->parID = desktopParams.ioAPPLParID;
                                applicationFSSpecPtr->vRefNum = desktopParams.ioVRefNum;
                                foundFlag = true;

                        }
                }

        SkipThisVolume:
				/*
                ** if retCode indicates a no such volume error or if this
                ** was the first pass, it's time to move on to the next pass
                */

                if (retCode == nsvErr) {
                        volumePass++;
                        volumeIndex = 0;
                }

        } while (foundFlag == false && volumePass != donePass);

Bail:
		if (retCode == nsvErr)
			return fnfErr;		/* More logical than "No such volume" */
        return retCode;
}
