/* GET FULL PATHNAME OF A FILE.
** Original by Guido, modified by Jack to handle FSSpecs 
*/

#include <string.h>

#include <Files.h>

#include "nfullpath.h"

/* Mac file system parameters */
#define MAXPATH 256	/* Max path name length+1 */
#define SEP ':'		/* Separator in path names */

/* Macro to find out whether we can do HFS-only calls: */
#define FSFCBLen (* (short *) 0x3f6)
#define hfsrunning() (FSFCBLen > 0)

int
nfullpath(fsp, retbuf)
	FSSpec *fsp;
	char *retbuf;
{
	union {
		HFileInfo f;
		DirInfo d;
		WDPBRec w;
		VolumeParam v;
	} pb;
	char cwd[2*MAXPATH];
	unsigned char namebuf[MAXPATH];
	short err;
	int dir;
	long dirid;
	char *next = cwd + sizeof cwd - 1;
	int len;
	int need_sep = 1;
	
	if (!hfsrunning())
		return -1;
	
	dir = fsp->vRefNum;
	dirid = fsp->parID;
	/* Stuff the filename into the buffer */
	len = fsp->name[0];
	*next = '\0';
	next -= len;
	memcpy(next, fsp->name+1, len);
	
	while (dirid != fsRtParID) {
		pb.d.ioNamePtr = namebuf;
		pb.d.ioVRefNum = dir;
		pb.d.ioFDirIndex = -1;
		pb.d.ioDrDirID = dirid;
		err= PBGetCatInfo((CInfoPBPtr)&pb.d, 0);
		if (err != noErr)
			return err;
		*--next = SEP;
		len = namebuf[0];
		if ( len + strlen(next) >= MAXPATH )
			return -1;
		next -= len;
		memcpy(next, (char *)namebuf+1, len);
		dirid = pb.d.ioDrParID;
		need_sep = 0;
	}
	
	strcpy(retbuf, next);
	if (need_sep) {
		next = strchr(retbuf, '\0');
		*next++ = SEP;
		*next++ = '\0';
	}
	return 0;
}
