/*
 * Macintosh version of UNIX directory access package
 * (opendir, readdir, closedir).
 * Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
 */

#include "dirent.h"
#include "macdefs.h"

static DIR opened;

/*
 * Open a directory.  This means calling PBOpenWD.
 * The value returned is always the address of opened, or NULL.
 * (I have as yet no use for multiple open directories; this could
 * be implemented by allocating memory dynamically.)
 */

DIR *
opendir(path)
	char *path;
{
	union {
		WDPBRec d;
		VolumeParam v;
	} pb;
	char ppath[MAXPATH];
	short err;
	
	if (opened.nextfile != 0) {
		errno = EBUSY;
		return NULL; /* A directory is already open. */
	}
	strncpy(ppath+1, path, ppath[0]= strlen(path));
	pb.d.ioNamePtr= (unsigned char *)ppath;
	pb.d.ioVRefNum= 0;
	if (hfsrunning()) {
		pb.d.ioWDProcID= 0;
		pb.d.ioWDDirID= 0;
		err= PBOpenWD((WDPBPtr)&pb, FALSE);
	}
	else {
		pb.v.ioVolIndex= 0;
		err= PBGetVInfo((ParmBlkPtr)&pb, FALSE);
	}
	if (err != noErr) {
		errno = ENOENT;
		return NULL;
	}
	opened.dirid= pb.d.ioVRefNum;
	opened.nextfile= 1;
	return &opened;
}

/*
 * Close a directory.
 */

void
closedir(dirp)
	DIR *dirp;
{
	if (hfsrunning()) {
		WDPBRec pb;
		
		pb.ioVRefNum= dirp->dirid;
		(void) PBCloseWD(&pb, FALSE);
	}
	dirp->dirid= 0;
	dirp->nextfile= 0;
}

/*
 * Read the next directory entry.
 */

struct dirent *
readdir(dp)
	DIR *dp;
{
	union {
		DirInfo d;
		FileParam f;
		HFileInfo hf;
	} pb;
	short err;
	static struct dirent dir;
	
	dir.d_name[0]= 0;
	pb.d.ioNamePtr= (unsigned char *)dir.d_name;
	pb.d.ioVRefNum= dp->dirid;
	pb.d.ioFDirIndex= dp->nextfile++;
	pb.d.ioDrDirID= 0;
	if (hfsrunning())
		err= PBGetCatInfo((CInfoPBPtr)&pb, FALSE);
	else
		err= PBGetFInfo((ParmBlkPtr)&pb, FALSE);
	if (err != noErr) {
		errno = EIO;
		return NULL;
	}
	(void) p2cstr((unsigned char *)dir.d_name);
	return &dir;
}
