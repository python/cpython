/* Get full pathname of current working directory.  The pathname is
   copied to the parameter array 'cwd', and a pointer to this array
   is also returned as function result.  If an error occurred, however,
   the return value is NULL but 'cwd' is filled with an error message.
   
   BUG: expect spectacular crashes when called from a directory whose
   path would be over MAXPATH bytes long (files in such directories are
   not reachable by full pathname).
   
   Starting with the dir ID returned by PBHGetVol, we do successive
   PBGetCatInfo's to get a component of the path until we reach the
   root (recognized by a dir ID of 2).  We move up along the path
   using the dir ID of the parent directory returned by PBGetCatInfo.
   
   Then we catenate the components found in reverse order with the volume
   name (already gotten from PBHGetVol), with intervening and trailing
   colons
   
   The code works correctly on MFS disks (where it always returns the
   volume name) by simply skipping the PBGetCatinfo calls in that case.
   There is a 'bug' in PBGetCatInfo when called for an MFS disk (with
   HFS running): it then seems to call PBHGetVInfo, which returns a
   larger parameter block.  But we won't run into this problem because
   we never call PBGetCatInfo for the root (assuming that PBHGetVol
   still sets the root ID in this case).

   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
*/

#include "macdefs.h"

#define ROOTID 2 /* Root directory ID */

char *
getwd(cwd)
	char *cwd;
{
	/* Universal parameter block. */
	union {
#ifdef THINK_C
		HFileInfo f;
		DirInfo d;
		WDPBRec w;
#else /* MPW */
		struct HFileInfo f;
		struct DirInfo d;
		struct WDPBRec w;
#endif
	} pb;
	char buf[MAXPATH]; /* Buffer to store the name components */
	char *ecwd, *ebuf; /* Pointers to end of used part of cwd and buf */
	int err; /* Error code of last I/O call */
	
	/* First, get the default volume name and working directory ID. */
	
	pb.w.ioNamePtr= (unsigned char *)cwd;
	err= PBHGetVolSync(&pb.w);
	if (err != noErr) {
		sprintf(cwd, "I/O error %d in PBHGetVolSync", err);
		return NULL;
	}
	ecwd= strchr((const char *)p2cstr((unsigned char*)cwd), EOS);
	ebuf= buf;
	*ebuf = EOS;
	
	/* Next, if at least we're running HFS, walk up the path. */
	
	if (hfsrunning()) {
		long dirid= pb.w.ioWDDirID;
		pb.d.ioVRefNum= pb.w.ioWDVRefNum;
		while (dirid != ROOTID) {
			pb.d.ioNamePtr= (unsigned char *) ++ebuf;
			pb.d.ioFDirIndex= -1;
			pb.d.ioDrDirID= dirid;
			err= PBGetCatInfoSync((CInfoPBPtr)&pb.d);
			if (err != noErr) {
				sprintf(cwd, "I/O error %d in PBGetCatInfoSync", err);
				return NULL;
			}
			dirid= pb.d.ioDrParID;
			ebuf += strlen((const char *)p2cstr((unsigned char *)ebuf));
			/* Should check for buf overflow */
		}
	}
	
	/* Finally, reverse the list of components and append it to cwd.
	   Ebuf points at the EOS after last component,
	   and there is an EOS before the first component.
	   If there are no components, ebuf equals buf (but there
	   is still an EOS where it points).
	   Ecwd points at the EOS after the path built up so far,
	   initially the volume name.
	   We break out of the loop in the middle, thus
	   appending a colon at the end in all cases. */
	
	for (;;) {
		*ecwd++ = ':';
		if (ebuf == buf)
			break;
		do { } while (*--ebuf != EOS); /* Find component start */
		strcpy(ecwd, ebuf+1);
		ecwd= strchr(ecwd, EOS);
	}
	*ecwd= EOS;
	return cwd;
}
