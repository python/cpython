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
#if TARGET_API_MAC_CARBON
	Str255 ppath;
	FSSpec fss;
	int plen;
	OSErr err;
	
	if (opened.nextfile != 0) {
		errno = EBUSY;
		return NULL; /* A directory is already open. */
	}
	plen = strlen(path);
	c2pstrcpy(ppath, path);
	if ( ppath[plen] != ':' )
		ppath[++plen] = ':';
	ppath[++plen] = 'x';
	ppath[0] = plen;
	if( (err = FSMakeFSSpec(0, 0, ppath, &fss)) < 0 && err != fnfErr ) {
		errno = EIO;
		return NULL;
	}
	opened.dirid = fss.parID;
	opened.vrefnum = fss.vRefNum;
	opened.nextfile = 1;
	return &opened;
#else
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
	pb.d.ioWDProcID= 0;
	pb.d.ioWDDirID= 0;
	err= PBOpenWD((WDPBPtr)&pb, 0);
	if (err != noErr) {
		errno = ENOENT;
		return NULL;
	}
	opened.dirid= pb.d.ioVRefNum;
	opened.nextfile= 1;
	return &opened;
#endif
}

/*
 * Close a directory.
 */

void
closedir(dirp)
	DIR *dirp;
{
#if TARGET_API_MAC_CARBON
	dirp->nextfile = 0;
#else
	WDPBRec pb;
	
	pb.ioVRefNum= dirp->dirid;
	(void) PBCloseWD(&pb, 0);
	dirp->dirid= 0;
	dirp->nextfile= 0;
#endif
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
#if TARGET_API_MAC_CARBON
	pb.d.ioVRefNum= dp->vrefnum;
	pb.d.ioDrDirID= dp->dirid;
#else
	pb.d.ioVRefNum= dp->dirid;
	pb.d.ioDrDirID= 0;
#endif
	pb.d.ioFDirIndex= dp->nextfile++;
	err= PBGetCatInfo((CInfoPBPtr)&pb, 0);
	if (err != noErr) {
		errno = EIO;
		return NULL;
	}
#if TARGET_API_MAC_CARBON
	p2cstrcpy(dir.d_name, (StringPtr)dir.d_name);
#else
	(void) p2cstr((unsigned char *)dir.d_name);
#endif
	return &dir;
}
