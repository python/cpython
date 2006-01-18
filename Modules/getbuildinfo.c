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

#ifdef SUBWCREV
#define SVNVERSION "$WCRANGE$$WCMODS?M:$"
#endif

const char *
Py_GetBuildInfo(void)
{
	static char buildinfo[50];
	const char *revision = Py_SubversionRevision();
	const char *sep = *revision ? ":" : "";
	const char *branch = Py_SubversionShortBranch();
	PyOS_snprintf(buildinfo, sizeof(buildinfo),
		      "%s%s%s, %.20s, %.9s", branch, sep, revision, 
		      DATE, TIME);
	return buildinfo;
}

const char *
_Py_svnversion(void)
{
#ifdef SVNVERSION
	return SVNVERSION;
#else
	return "exported";
#endif
}
