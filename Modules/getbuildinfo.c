#include "config.h"

#include <stdio.h>

#ifndef DATE
#ifdef __DATE__
#define DATE __DATE__
#else
#define DATE "xx/xx/xx"
#endif
#endif

#ifndef TIME
#ifdef __TIME__
#define TIME __TIME__
#else
#define TIME "xx:xx:xx"
#endif
#endif

#ifndef BUILD
#define BUILD 0
#endif


const char *
Py_GetBuildInfo()
{
	static char buildinfo[40];
	sprintf(buildinfo, "#%d, %.12s, %.8s", BUILD, DATE, TIME);
	return buildinfo;
}
