/* Return the name of the boot volume (not the current directory).
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
*/

#include "macdefs.h"

char *
getbootvol(void)
{
	short vrefnum;
	static unsigned char name[32];
	
	(void) GetVol(name, &vrefnum);
	p2cstr(name);
		/* Shouldn't fail; return ":" if it does */
	strcat((char *)name, ":");
	return (char *)name;
}
