/* Return the full version string. */

#include "patchlevel.h"

#define VERSION "%s (%s) %s"

#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "August 1 1995"
#endif

extern const char *getcompiler();

const char *
getversion()
{
	static char version[80];
	sprintf(version, VERSION, PATCHLEVEL, DATE, getcompiler());
	return version;
}
