/* Mkdir for the Macintosh.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
   Pathnames must be Macintosh paths, with colons as separators. */

#include "macdefs.h"

/* Create a directory. */

int
mkdir(path, mode)
	char *path;
	int mode; /* Ignored */
{
	HFileParam pb;
	char name[MAXPATH];
		
	if (!hfsrunning()) {
		errno= ENODEV;
		return -1;
	}
	strncpy(name, path, sizeof name);
	pb.ioNamePtr= (StringPtr) c2pstr(name);
	pb.ioVRefNum= 0;
	pb.ioDirID= 0;
	if (PBDirCreate((HParmBlkPtr)&pb, FALSE) != noErr) {
		errno= EACCES;
		return -1;
	}
	return 0;
}
