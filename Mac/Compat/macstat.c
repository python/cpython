/* Minimal 'stat' emulation: tells directories from files and
   gives length and mtime.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
*/

#include "stat.h"
#include "macdefs.h"

/* Bits in ioFlAttrib: */
#define LOCKBIT	(1<<0)		/* File locked */
#define DIRBIT	(1<<4)		/* It's a directory */

int
stat(path, buf)
	char *path;
	struct stat *buf;
{
	union {
		DirInfo d;
		FileParam f;
		HFileInfo hf;
	} pb;
	char name[256];
	short err;
	
	pb.d.ioNamePtr= (unsigned char *)c2pstr(strcpy(name, path));
	pb.d.ioVRefNum= 0;
	pb.d.ioFDirIndex= 0;
	pb.d.ioDrDirID= 0;
	pb.f.ioFVersNum= 0; /* Fix found by Timo! See Tech Note 102 */
	if (hfsrunning())
		err= PBGetCatInfo((CInfoPBPtr)&pb, FALSE);
	else
		err= PBGetFInfo((ParmBlkPtr)&pb, FALSE);
	if (err != noErr) {
		errno = ENOENT;
		return -1;
	}
	if (pb.d.ioFlAttrib & LOCKBIT)
		buf->st_mode= 0444;
	else
		buf->st_mode= 0666;
	if (pb.d.ioFlAttrib & DIRBIT) {
		buf->st_mode |= 0111 | S_IFDIR;
		buf->st_size= pb.d.ioDrNmFls;
		buf->st_rsize= 0;
	}
	else {
		buf->st_mode |= S_IFREG;
		if (pb.f.ioFlFndrInfo.fdType == 'APPL')
			buf->st_mode |= 0111;
		buf->st_size= pb.f.ioFlLgLen;
		buf->st_rsize= pb.f.ioFlRLgLen;
	}
	buf->st_mtime= pb.f.ioFlMdDat - TIMEDIFF;
	return 0;
}
