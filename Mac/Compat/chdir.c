/* Chdir for the Macintosh.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
   Pathnames must be Macintosh paths, with colons as separators. */

#include "macdefs.h"

#ifdef __MWERKS__
/* XXXX All compilers should use this, really */
#include <LowMem.h>
#else
/* Last directory used by Standard File */
#define SFSaveDisk	(*(short *)0x214)
#define CurDirStore (*(long *)0x398)
#endif

/* Change current directory. */

int
chdir(path)
	char *path;
{
	WDPBRec pb;
	
	pb.ioNamePtr= (StringPtr) Pstring(path);
	pb.ioVRefNum= 0;
	pb.ioWDDirID= 0;
	if (PBHSetVol(&pb, FALSE) != noErr) {
		errno= ENOENT;
		return -1;
	}
	if (PBHGetVol(&pb, FALSE) == noErr) {
		/* Set the Standard File directory */
#ifdef __MWERKS__
		LMSetSFSaveDisk(-pb.ioWDVRefNum);
		LMSetCurDirStore(pb.ioWDDirID);
#else
		SFSaveDisk= -pb.ioWDVRefNum;
		CurDirStore= pb.ioWDDirID;
#endif
	}
	return 0;
}
