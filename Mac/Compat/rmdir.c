/* Rmdir for the Macintosh.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
   Pathnames must be Macintosh paths, with colons as separators. */

#include "macdefs.h"

int
rmdir(path)
	char *path;
{
	IOParam pb;
	
	pb.ioNamePtr= (StringPtr) Pstring(path);
	pb.ioVRefNum= 0;
	if (PBDelete((ParmBlkPtr)&pb, FALSE) != noErr) {
		errno= EACCES;
		return -1;
	}
	return 0;
}
