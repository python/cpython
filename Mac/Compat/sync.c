/* The equivalent of the Unix 'sync' system call: FlushVol.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
   For now, we only flush the default volume
   (since that's the only volume written to by MacB). */

#include "macdefs.h"

int
sync()
{
	if (FlushVol((StringPtr)0, 0) == noErr)
		return 0;
	else {
		errno= ENODEV;
		return -1;
	}
}
