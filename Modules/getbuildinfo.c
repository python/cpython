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
#ifndef PY_BUILD_STR
   // On Windows, only provide basic info
#  ifdef Py_DEBUG
#    define PY_BUILD_STR "debug build"
#  else
#    define PY_BUILD_STR "release build"
#  endif
#endif

// len(", release+assert+tracerefs+pystats asan msan ubsan lto=thin+pgo framework shared build")
#define PY_BUILD_STR_MAXLEN 86

static int initialized = 0;
static char buildinfo[50 + sizeof(GITVERSION) +
                      ((sizeof(GITTAG) > sizeof(GITBRANCH)) ?
                       sizeof(GITTAG) : sizeof(GITBRANCH))
                       + PY_BUILD_STR_MAXLEN];

const char *
Py_GetBuildInfo(void)
{
    if (initialized) {
        return buildinfo;
    }
    initialized = 1;
    const char *revision = _Py_gitversion();
    const char *sep = *revision ? ":" : "";
    const char *gitid = _Py_gitidentifier();
    if (!(*gitid)) {
        gitid = "main";
    }
    PyOS_snprintf(buildinfo, sizeof(buildinfo),
                  "%s%s%s, %.20s, %.9s, %s",
                gitid, sep, revision, DATE, TIME,
                PY_BUILD_STR);
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
