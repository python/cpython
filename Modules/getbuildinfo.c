#include "Python.h"

#ifndef DONT_HAVE_STDIO_H
#include <stdio.h>
#endif

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
#define BUILD "$Revision$"
#endif

const char *
Py_GetBuildNumber(void)
{
	static char buildno[20];
	static int buildno_okay;

	if (!buildno_okay) {
		char *build = BUILD;
		int len = strlen(build);

		if (len > 13 &&
		    !strncmp(build, "$Revision: ", 11) &&
		    !strcmp(build + len - 2, " $"))
		{
			memcpy(buildno, build + 11, len - 13);
		}
		else {
			memcpy(buildno, build, 19);
		}
		buildno_okay = 1;
	}
	return buildno;
}

const char *
Py_GetBuildInfo(void)
{
	static char buildinfo[50];
	PyOS_snprintf(buildinfo, sizeof(buildinfo),
		      "#%s, %.20s, %.9s", Py_GetBuildNumber(), DATE, TIME);
	return buildinfo;
}
