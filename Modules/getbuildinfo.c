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

/* XXX Only unix build process has been tested */
#ifndef GITVERSION
#define GITVERSION ""
#endif
#ifndef GITTAG
#define GITTAG ""
#endif
#ifndef GITBRANCH
#define GITBRANCH ""
#endif

const char *
Py_GetBuildInfo(void)
{
    static char buildinfo[50 + sizeof(GITVERSION) +
                          ((sizeof(GITTAG) > sizeof(GITBRANCH)) ?
                           sizeof(GITTAG) : sizeof(GITBRANCH))];
    const char *revision = _Py_gitversion();
    const char *sep = *revision ? ":" : "";
    const char *gitid = _Py_gitidentifier();
    if (!(*gitid)) {
        gitid = "main";
    }
    PyOS_snprintf(buildinfo, sizeof(buildinfo),
                  "%s%s%s, %.20s, %.9s", gitid, sep, revision,
                  DATE, TIME);
    return buildinfo;
}

const char *
_Py_gitversion(void)
{
    return GITVERSION;
}

const char *
_Py_gitidentifier(void)
{
    const char *gittag, *gitid;
    gittag = GITTAG;
    if ((*gittag) && strcmp(gittag, "undefined") != 0)
        gitid = gittag;
    else
        gitid = GITBRANCH;
    return gitid;
}
