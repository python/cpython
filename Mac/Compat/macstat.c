/* Minimal 'stat' emulation: tells directories from files and
   gives length and mtime.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
   Updated to give more info, August 1994.
*/

#include "macstat.h"
#include "macdefs.h"

/* Bits in ioFlAttrib: */
#define LOCKBIT	(1<<0)		/* File locked */
#define DIRBIT	(1<<4)		/* It's a directory */

int
macstat(path, buf)
	char *path;
	struct macstat *buf;
{
	union {
		DirInfo d;
		FileParam f;
		HFileInfo hf;
	} pb;
	short err;
	
	pb.d.ioNamePtr = (unsigned char *)Pstring(path);
	pb.d.ioVRefNum = 0;
	pb.d.ioFDirIndex = 0;
	pb.d.ioDrDirID = 0;
	pb.f.ioFVersNum = 0; /* Fix found by Timo! See Tech Note 102 */
	if (hfsrunning())
		err = PBGetCatInfo((CInfoPBPtr)&pb, FALSE);
	else
		err = PBGetFInfo((ParmBlkPtr)&pb, FALSE);
	if (err != noErr) {
		errno = ENOENT;
		return -1;
	}
	if (pb.d.ioFlAttrib & LOCKBIT)
		buf->st_mode = 0444;
	else
		buf->st_mode = 0666;
	if (pb.d.ioFlAttrib & DIRBIT) {
		buf->st_mode |= 0111 | S_IFDIR;
		buf->st_size = pb.d.ioDrNmFls;
		buf->st_rsize = 0;
	}
	else {
		buf->st_mode |= S_IFREG;
		if (pb.f.ioFlFndrInfo.fdType == 'APPL')
			buf->st_mode |= 0111;
	}
	buf->st_ino = pb.hf.ioDirID;
	buf->st_nlink = 1;
	buf->st_uid = 1;
	buf->st_gid = 1;
	buf->st_size = pb.f.ioFlLgLen;
	buf->st_mtime = buf->st_atime = pb.f.ioFlMdDat;
	buf->st_ctime = pb.f.ioFlCrDat;
	buf->st_rsize = pb.f.ioFlRLgLen;
	*(unsigned long *)buf->st_type = pb.f.ioFlFndrInfo.fdType;
	*(unsigned long *)buf->st_creator = pb.f.ioFlFndrInfo.fdCreator;
	return 0;
}
